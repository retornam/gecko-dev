# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import ast
import os
import sys
from collections import OrderedDict

import config


def abs_path(path):
    return os.path.abspath(os.path.expanduser(path))


def url_or_path(path):
    import urlparse

    parsed = urlparse.urlparse(path)
    if len(parsed.scheme) > 2:
        return path
    else:
        return abs_path(path)

def slash_prefixed(url):
    if not url.startswith("/"):
        url = "/" + url
    return url


def require_arg(kwargs, name, value_func=None):
    if value_func is None:
        value_func = lambda x: x is not None

    if not name in kwargs or not value_func(kwargs[name]):
        print >> sys.stderr, "Missing required argument %s" % name
        sys.exit(1)


def create_parser(product_choices=None):
    from mozlog.structured import commandline

    import products

    if product_choices is None:
        config_data = config.load()
        product_choices = products.products_enabled(config_data)

    parser = argparse.ArgumentParser(description="Runner for web-platform-tests tests.")
    parser.add_argument("--metadata", action="store", type=abs_path, dest="metadata_root",
                        help="Path to the folder containing test metadata"),
    parser.add_argument("--tests", action="store", type=abs_path, dest="tests_root",
                        help="Path to test files"),
    parser.add_argument("--prefs-root", dest="prefs_root", action="store", type=abs_path,
                        help="Path to the folder containing browser prefs"),
    parser.add_argument("--serve-root", action="store", type=abs_path, dest="serve_root",
                        help="Path to web-platform-tests checkout containing serve.py and manifest.py"
                        " (defaults to test_root)")
    parser.add_argument("--run-info", action="store", type=abs_path,
                        help="Path to directory containing extra json files to add to run info")
    parser.add_argument("--config", action="store", type=abs_path, dest="config",
                        help="Path to config file")

    parser.add_argument("--manifest-update", action="store_true", default=False,
                        help="Force regeneration of the test manifest")

    parser.add_argument("--binary", action="store",
                        type=abs_path, help="Binary to run tests against")
    parser.add_argument("--webdriver-binary", action="store", metavar="BINARY",
                        type=abs_path, help="WebDriver server binary to use")
    parser.add_argument("--test-types", action="store",
                        nargs="*", default=["testharness", "reftest"],
                        choices=["testharness", "reftest"],
                        help="Test types to run")
    parser.add_argument("--processes", action="store", type=int, default=1,
                        help="Number of simultaneous processes to use")
    parser.add_argument("--include", action="append", type=slash_prefixed,
                        help="URL prefix to include")
    parser.add_argument("--exclude", action="append", type=slash_prefixed,
                        help="URL prefix to exclude")
    parser.add_argument("--include-manifest", type=abs_path,
                        help="Path to manifest listing tests to include")

    parser.add_argument("--run-by-dir", type=int, nargs="?", default=False,
                        help="Split run into groups by directories. With a parameter,"
                        "limit the depth of splits e.g. --run-by-dir=1 to split by top-level"
                        "directory")

    parser.add_argument("--total-chunks", action="store", type=int, default=1,
                        help="Total number of chunks to use")
    parser.add_argument("--this-chunk", action="store", type=int, default=1,
                        help="Chunk number to run")
    parser.add_argument("--chunk-type", action="store", choices=["none", "equal_time", "hash"],
                        default=None, help="Chunking type to use")

    parser.add_argument("--list-test-groups", action="store_true",
                        default=False,
                        help="List the top level directories containing tests that will run.")
    parser.add_argument("--list-disabled", action="store_true",
                        default=False,
                        help="List the tests that are disabled on the current platform")

    parser.add_argument("--timeout-multiplier", action="store", type=float, default=None,
                        help="Multiplier relative to standard test timeout to use")
    parser.add_argument("--repeat", action="store", type=int, default=1,
                        help="Number of times to run the tests")

    parser.add_argument("--no-capture-stdio", action="store_true", default=False,
                        help="Don't capture stdio and write to logging")

    parser.add_argument("--product", action="store", choices=product_choices,
                        default="firefox", help="Browser against which to run tests")

    parser.add_argument('--debugger',
                        help="run under a debugger, e.g. gdb or valgrind")
    parser.add_argument('--debugger-args', help="arguments to the debugger")
    parser.add_argument('--pause-on-unexpected', action="store_true",
                        help="Halt the test runner when an unexpected result is encountered")

    parser.add_argument("--symbols-path", action="store", type=url_or_path,
                        help="Path or url to symbols file used to analyse crash minidumps.")
    parser.add_argument("--stackwalk-binary", action="store", type=abs_path,
                        help="Path to stackwalker program used to analyse minidumps.")

    parser.add_argument("--b2g-no-backup", action="store_true", default=False,
                        help="Don't backup device before testrun with --product=b2g")

    commandline.add_logging_group(parser)
    return parser


def set_from_config(kwargs):
    if kwargs["config"] is None:
        config_path = config.path()
    else:
        config_path = kwargs["config"]

    kwargs["config_path"] = config_path
    kwargs["config"] = config.read(kwargs["config_path"])

    keys = {"paths": [("serve", "serve_root", True),
                      ("prefs", "prefs_root", True),
                      ("run_info", "run_info", True)],
            "web-platform-tests": [("remote_url", "remote_url", False),
                                   ("branch", "branch", False),
                                   ("sync_path", "sync_path", True)]}

    for section, values in keys.iteritems():
        for config_value, kw_value, is_path in values:
            if kw_value in kwargs and kwargs[kw_value] is None:
                if not is_path:
                    new_value = kwargs["config"].get(section, {}).get(config_value)
                else:
                    new_value = kwargs["config"].get(section, {}).get_path(config_value)
                kwargs[kw_value] = new_value

    kwargs["test_paths"] = get_test_paths(kwargs["config"])

    if kwargs["tests_root"]:
        if "/" not in kwargs["test_paths"]:
            kwargs["test_paths"]["/"] = {}
        kwargs["test_paths"]["/"]["tests_path"] = kwargs["tests_root"]

    if kwargs["metadata_root"]:
        if "/" not in kwargs["test_paths"]:
            kwargs["test_paths"]["/"] = {}
        kwargs["test_paths"]["/"]["metadata_path"] = kwargs["metadata_root"]

def get_test_paths(config):
    # Set up test_paths
    test_paths = OrderedDict()

    for section in config.iterkeys():
        if section.startswith("manifest:"):
            manifest_opts = config.get(section)
            url_base = manifest_opts.get("url_base", "/")
            test_paths[url_base] = {
                "tests_path": manifest_opts.get_path("tests"),
                "metadata_path": manifest_opts.get_path("metadata")}

    return test_paths



def check_args(kwargs):
    from mozrunner import debugger_arguments

    set_from_config(kwargs)

    for test_paths in kwargs["test_paths"].itervalues():
        if not ("tests_path" in test_paths and
                "metadata_path" in test_paths):
            print "Fatal: must specify both a test path and metadata path"
            sys.exit(1)
        for key, path in test_paths.iteritems():
            name = key.split("_", 1)[0]

            if not os.path.exists(path):
                print "Fatal: %s path %s does not exist" % (name, path)
                sys.exit(1)

            if not os.path.isdir(path):
                print "Fatal: %s path %s is not a directory" % (name, path)
                sys.exit(1)

    if kwargs["serve_root"] is None:
        if "/" in kwargs["test_paths"]:
            kwargs["serve_root"] = kwargs["test_paths"]["/"]["tests_path"]
    else:
        print >> sys.stderr, "Unable to determine server root path"
        sys.exit(1)

    if kwargs["run_info"] is None:
        kwargs["run_info"] = kwargs["config_path"]

    if kwargs["this_chunk"] > 1:
        require_arg(kwargs, "total_chunks", lambda x: x >= kwargs["this_chunk"])

    if kwargs["chunk_type"] is None:
        if kwargs["total_chunks"] > 1:
            kwargs["chunk_type"] = "equal_time"
        else:
            kwargs["chunk_type"] = "none"

    if kwargs["debugger"] is not None:
        debug_args, interactive = debugger_arguments(kwargs["debugger"],
                                                     kwargs["debugger_args"])
        if interactive:
            require_arg(kwargs, "processes", lambda x: x == 1)
            kwargs["no_capture_stdio"] = True
        kwargs["interactive"] = interactive
        kwargs["debug_args"] = debug_args
    else:
        kwargs["interactive"] = False
        kwargs["debug_args"] = None

    if kwargs["binary"] is not None:
        if not os.path.exists(kwargs["binary"]):
            print >> sys.stderr, "Binary path %s does not exist" % kwargs["binary"]
            sys.exit(1)

    return kwargs


def create_parser_update():
    parser = argparse.ArgumentParser("web-platform-tests-update",
                                     description="Update script for web-platform-tests tests.")
    parser.add_argument("--config", action="store", type=abs_path, help="Path to config file")
    parser.add_argument("--metadata", action="store", type=abs_path, dest="metadata_root",
                        help="Path to the folder containing test metadata"),
    parser.add_argument("--tests", action="store", type=abs_path, dest="tests_root",
                        help="Path to web-platform-tests"),
    parser.add_argument("--sync-path", action="store", type=abs_path,
                        help="Path to store git checkout of web-platform-tests during update"),
    parser.add_argument("--serve-root", action="store", type=abs_path, dest="serve_root",
                        help="Path to web-platform-tests checkout containing serve.py and manifest.py"
                        " (defaults to test_root)")
    parser.add_argument("--remote_url", action="store",
                        help="URL of web-platfrom-tests repository to sync against"),
    parser.add_argument("--branch", action="store", type=abs_path,
                        help="Remote branch to sync against")
    parser.add_argument("--rev", action="store", help="Revision to sync to")
    parser.add_argument("--no-check-clean", action="store_true", default=False,
                        help="Don't check the working directory is clean before updating")
    parser.add_argument("--patch", action="store_true",
                        help="Create an mq patch or git commit containing the changes.")
    parser.add_argument("--sync", dest="sync", action="store_true", default=False,
                        help="Sync the tests with the latest from upstream")
    parser.add_argument("--ignore-existing", action="store_true", help="When updating test results only consider results from the logfiles provided, not existing expectations.")
    # Should make this required iff run=logfile
    parser.add_argument("run_log", nargs="*", type=abs_path,
                        help="Log file from run of tests")
    return parser


def create_parser_reduce(product_choices=None):
    parser = create_parser(product_choices)
    parser.add_argument("target", action="store", help="Test id that is unstable")
    return parser


def parse_args():
    parser = create_parser()
    rv = vars(parser.parse_args())
    check_args(rv)
    return rv


def parse_args_update():
    parser = create_parser_update()
    rv = vars(parser.parse_args())
    set_from_config(rv)
    return rv


def parse_args_reduce():
    parser = create_parser_reduce()
    rv = vars(parser.parse_args())
    check_args(rv)
    return rv
