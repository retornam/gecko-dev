/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

/**
 * This file contains the "markers overview" graph, which is a minimap of all
 * the timeline data. Regions inside it may be selected, determining which
 * markers are visible in the "waterfall".
 */

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/Graphs.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "L10N",
  "devtools/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/timeline/global", true);

const HTML_NS = "http://www.w3.org/1999/xhtml";

const OVERVIEW_HEADER_HEIGHT = 14; // px
const OVERVIEW_BODY_HEIGHT = 55; // 11px * 5 groups

const OVERVIEW_BACKGROUND_COLOR = "#fff";
const OVERVIEW_CLIPHEAD_LINE_COLOR = "#666";
const OVERVIEW_SELECTION_LINE_COLOR = "#555";
const OVERVIEW_SELECTION_BACKGROUND_COLOR = "rgba(76,158,217,0.25)";
const OVERVIEW_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";

const OVERVIEW_HEADER_TICKS_MULTIPLE = 100; // ms
const OVERVIEW_HEADER_TICKS_SPACING_MIN = 75; // px
const OVERVIEW_HEADER_SAFE_BOUNDS = 50; // px
const OVERVIEW_HEADER_BACKGROUND = "#fff";
const OVERVIEW_HEADER_TEXT_COLOR = "#18191a";
const OVERVIEW_HEADER_TEXT_FONT_SIZE = 9; // px
const OVERVIEW_HEADER_TEXT_FONT_FAMILY = "sans-serif";
const OVERVIEW_HEADER_TEXT_PADDING_LEFT = 6; // px
const OVERVIEW_HEADER_TEXT_PADDING_TOP = 1; // px
const OVERVIEW_TIMELINE_STROKES = "#ccc";
const OVERVIEW_MARKERS_COLOR_STOPS = [0, 0.1, 0.75, 1];
const OVERVIEW_MARKER_DURATION_MIN = 4; // ms
const OVERVIEW_GROUP_VERTICAL_PADDING = 5; // px
const OVERVIEW_GROUP_ALTERNATING_BACKGROUND = "rgba(0,0,0,0.05)";

/**
 * An overview for the markers data.
 *
 * @param nsIDOMNode parent
 *        The parent node holding the overview.
 */
function MarkersOverview(parent, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "markers-overview", ...args]);
  this.once("ready", () => {
    // Set the list of names, properties and colors used to paint this overview.
    this.setBlueprint(TIMELINE_BLUEPRINT);

    // Populate this overview with some dummy initial data.
    this.setData({ interval: { startTime: 0, endTime: 1000 }, markers: [] });
  });
}

MarkersOverview.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  clipheadLineColor: OVERVIEW_CLIPHEAD_LINE_COLOR,
  selectionLineColor: OVERVIEW_SELECTION_LINE_COLOR,
  selectionBackgroundColor: OVERVIEW_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: OVERVIEW_SELECTION_STRIPES_COLOR,
  headerHeight: OVERVIEW_HEADER_HEIGHT,
  bodyHeight: OVERVIEW_BODY_HEIGHT,
  groupPadding: OVERVIEW_GROUP_VERTICAL_PADDING,

  /**
   * Compute the height of the overview.
   */
  get fixedHeight() {
    return this.headerHeight + this.bodyHeight;
  },

  /**
   * List of names and colors used to paint this overview.
   * @see TIMELINE_BLUEPRINT in timeline/widgets/global.js
   */
  setBlueprint: function(blueprint) {
    this._paintBatches = new Map();
    this._lastGroup = 0;

    for (let type in blueprint) {
      this._paintBatches.set(type, { style: blueprint[type], batch: [] });
      this._lastGroup = Math.max(this._lastGroup, blueprint[type].group);
    }
  },

  /**
   * Disables selection and empties this graph.
   */
  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData({ interval: { startTime: 0, endTime: 0 }, markers: [] });
  },

  /**
   * Renders the graph's data source.
   * @see AbstractCanvasGraph.prototype.buildGraphImage
   */
  buildGraphImage: function() {
    let { interval, markers } = this._data;
    let { startTime, endTime } = interval;

    let { canvas, ctx } = this._getNamedCanvas("markers-overview-data");
    let canvasWidth = this._width;
    let canvasHeight = this._height;
    let safeBounds = OVERVIEW_HEADER_SAFE_BOUNDS * this._pixelRatio;
    let availableWidth = canvasWidth - safeBounds;

    // Group markers into separate paint batches. This is necessary to
    // draw all markers sharing the same style at once.

    for (let marker of markers) {
      this._paintBatches.get(marker.name).batch.push(marker);
    }

    // Calculate each group's height, and the time-based scaling.

    let totalGroups = this._lastGroup + 1;
    let headerHeight = this.headerHeight * this._pixelRatio;
    let groupHeight = this.bodyHeight * this._pixelRatio / totalGroups;
    let groupPadding = this.groupPadding * this._pixelRatio;

    let totalTime = (endTime - startTime) || 0;
    let dataScale = this.dataScaleX = availableWidth / totalTime;

    // Draw the header and overview background.

    ctx.fillStyle = OVERVIEW_HEADER_BACKGROUND;
    ctx.fillRect(0, 0, canvasWidth, headerHeight);

    ctx.fillStyle = OVERVIEW_BACKGROUND_COLOR;
    ctx.fillRect(0, headerHeight, canvasWidth, canvasHeight);

    // Draw the alternating odd/even group backgrounds.

    ctx.fillStyle = OVERVIEW_GROUP_ALTERNATING_BACKGROUND;
    ctx.beginPath();

    for (let i = 0; i < totalGroups; i += 2) {
      let top = headerHeight + i * groupHeight;
      ctx.rect(0, top, canvasWidth, groupHeight);
    }

    ctx.fill();

    // Draw the timeline header ticks.

    let fontSize = OVERVIEW_HEADER_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = OVERVIEW_HEADER_TEXT_FONT_FAMILY;
    let textPaddingLeft = OVERVIEW_HEADER_TEXT_PADDING_LEFT * this._pixelRatio;
    let textPaddingTop = OVERVIEW_HEADER_TEXT_PADDING_TOP * this._pixelRatio;
    let tickInterval = this._findOptimalTickInterval(dataScale);

    ctx.textBaseline = "middle";
    ctx.font = fontSize + "px " + fontFamily;
    ctx.fillStyle = OVERVIEW_HEADER_TEXT_COLOR;
    ctx.strokeStyle = OVERVIEW_TIMELINE_STROKES;
    ctx.beginPath();

    for (let x = 0; x < availableWidth; x += tickInterval) {
      let lineLeft = x;
      let textLeft = lineLeft + textPaddingLeft;
      let time = Math.round(x / dataScale);
      let label = L10N.getFormatStr("timeline.tick", time);
      ctx.fillText(label, textLeft, headerHeight / 2 + textPaddingTop);
      ctx.moveTo(lineLeft, 0);
      ctx.lineTo(lineLeft, canvasHeight);
    }

    ctx.stroke();

    // Draw the timeline markers.

    for (let [, { style, batch }] of this._paintBatches) {
      let top = headerHeight + style.group * groupHeight + groupPadding / 2;
      let height = groupHeight - groupPadding;

      let gradient = ctx.createLinearGradient(0, top, 0, top + height);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[0], style.stroke);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[1], style.fill);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[2], style.fill);
      gradient.addColorStop(OVERVIEW_MARKERS_COLOR_STOPS[3], style.stroke);
      ctx.fillStyle = gradient;
      ctx.beginPath();

      for (let { start, end } of batch) {
        start -= interval.startTime;
        end -= interval.startTime;

        let left = start * dataScale;
        let duration = Math.max(end - start, OVERVIEW_MARKER_DURATION_MIN);
        let width = Math.max(duration * dataScale, this._pixelRatio);
        ctx.rect(left, top, width, height);
      }

      ctx.fill();

      // Since all the markers in this batch (thus sharing the same style) have
      // been drawn, empty it. The next time new markers will be available,
      // they will be sorted and drawn again.
      batch.length = 0;
    }

    return canvas;
  },

  /**
   * Finds the optimal tick interval between time markers in this overview.
   */
  _findOptimalTickInterval: function(dataScale) {
    let timingStep = OVERVIEW_HEADER_TICKS_MULTIPLE;
    let spacingMin = OVERVIEW_HEADER_TICKS_SPACING_MIN * this._pixelRatio;

    while (true) {
      let scaledStep = dataScale * timingStep;
      if (scaledStep < spacingMin) {
        timingStep <<= 1;
        continue;
      }
      return scaledStep;
    }
  }
});

exports.MarkersOverview = MarkersOverview;
