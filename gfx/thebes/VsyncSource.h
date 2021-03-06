/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"

namespace mozilla {
class CompositorVsyncDispatcher;

namespace gfx {

// Controls how and when to enable/disable vsync. Lives as long as the
// gfxPlatform does on the parent process
class VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)
public:
  // Controls vsync unique to each display and unique on each platform
  class Display {
    public:
      Display();
      virtual ~Display();
      void AddCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void RemoveCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      // Notified when this display's vsync occurs, on the hardware vsync thread
      void NotifyVsync(mozilla::TimeStamp aVsyncTimestamp);

      // These should all only be called on the main thread
      virtual void EnableVsync() = 0;
      virtual void DisableVsync() = 0;
      virtual bool IsVsyncEnabled() = 0;

    private:
      nsTArray<nsRefPtr<mozilla::CompositorVsyncDispatcher>> mCompositorVsyncDispatchers;
  }; // end Display

  void AddCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  void RemoveCompositorVsyncDispatcher(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);

protected:
  virtual Display& GetGlobalDisplay() = 0; // Works across all displays
  virtual Display& FindDisplay(mozilla::CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  virtual ~VsyncSource() {}
}; // VsyncSource
} // gfx
} // mozilla
