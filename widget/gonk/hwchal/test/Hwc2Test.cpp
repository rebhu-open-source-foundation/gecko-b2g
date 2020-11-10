/*
 * Copyright (C) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED. All rights
 * reserved. Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <getopt.h>
#include <gtest/gtest.h>

#include <HWC2.h>
#include <ComposerHal.h>

#include <ui/GraphicBuffer.h>
#include <ui/Fence.h>

#include "Hwc2TestLayers.h"
#include "Hwc2TestClientTarget.h"

#define ASSERT_NO_HWCERROR(exp) ASSERT_EQ(exp, HWC2::Error::None)
#define EXPECT_NO_HWCERROR(exp) EXPECT_EQ(exp, HWC2::Error::None)

static bool gPrintValidateResult = false;

class Hwc2Test : public ::testing::Test, public HWC2::ComposerCallback {
 public:
  void onVsyncReceived(int32_t aSequenceId, hwc2_display_t aDisplay,
                       int64_t aTimestamp) override {
    mVsyncDisplay = aDisplay;
    mVsyncTimestamp = aTimestamp;
    mVsyncCv.notify_all();
  }

  void onHotplugReceived(int32_t aSequenceId, hwc2_display_t aDisplay,
                         HWC2::Connection aConnection) override {
    {
      std::lock_guard<std::mutex> lock(mHotplugMutex);
      mHwc->onHotplug(aDisplay, aConnection);

      if (aConnection == HWC2::Connection::Connected) {
        mDisplayIds.insert(aDisplay);
      } else if (aConnection == HWC2::Connection::Disconnected) {
        mDisplayIds.erase(aDisplay);
      }
    }
    mHotplugCv.notify_all();
  }

  void onRefreshReceived(int32_t aSequenceId,
                         hwc2_display_t aDisplay) override {
    // do nothing
  }

  void WaitForVsync(hwc2_display_t* outDisplay = nullptr,
                    int64_t* outTimestamp = nullptr) {
    std::unique_lock<std::mutex> lock(mVsyncMutex);
    ASSERT_EQ(mVsyncCv.wait_for(lock, std::chrono::seconds(3)),
              std::cv_status::no_timeout)
        << "timed out attempting to get"
           " vsync callback";

    if (outDisplay) {
      *outDisplay = mVsyncDisplay;
    }
    if (outTimestamp) {
      *outTimestamp = mVsyncTimestamp;
    }
  }

  Hwc2Test() {}

  virtual ~Hwc2Test() {}

  virtual void SetUp() {
    mHwc = std::make_unique<HWC2::Device>("default");
    mHwc->registerCallback(this, 0);
  }

  virtual void TearDown() {
    for (auto displayId : mDisplayIds) {
      if (auto display = mHwc->getDisplayById(displayId)) {
        display->setPowerMode(HWC2::PowerMode::Off);
      }
    }
  }

  HWC2::Display* EnsurePrimaryDisplay() {
    if (!mPrimaryDisplay) {
      std::unique_lock<std::mutex> lock(mHotplugMutex);

      mPrimaryDisplay = mHwc->getDisplayById(HWC_DISPLAY_PRIMARY);
      if (!mPrimaryDisplay) {
        /* Wait at most 5s for hotplug events */
        mHotplugCv.wait_for(lock, std::chrono::seconds(5));
        mPrimaryDisplay = mHwc->getDisplayById(HWC_DISPLAY_PRIMARY);
      }

      if (mPrimaryDisplay) {
        mPrimaryDisplay->setPowerMode(HWC2::PowerMode::On);
      }
    }

    return mPrimaryDisplay;
  }

  using TestDisplayLayersFunction = void (*)(
      Hwc2Test* aTest, HWC2::Display* aDisplay,
      const std::vector<HWC2::Layer*>& aLayers, Hwc2TestLayers* aTestLayers);

  Area GetActiveDisplayArea(
      std::shared_ptr<const HWC2::Display::Config> aConfig) {
    Area displayArea;
    displayArea.width = aConfig->getWidth();
    displayArea.height = aConfig->getHeight();
    return displayArea;
  }

  void CreateLayers(HWC2::Display* aDisplay, size_t aLayerCnt,
                    std::vector<HWC2::Layer*>* aOutLayers) {
    std::vector<HWC2::Layer*> newLayers;

    for (size_t i = 0; i < aLayerCnt; i++) {
      HWC2::Layer* layer;
      auto err = aDisplay->createLayer(&layer);

      if (err == HWC2::Error::NoResources) {
        break;
      }
      if (err != HWC2::Error::None) {
        newLayers.clear();
        ASSERT_EQ(err, HWC2::Error::None) << "failed to create layer";
      }
      newLayers.push_back(layer);
    }

    *aOutLayers = std::move(newLayers);
  }

  void DestroyLayers(HWC2::Display* aDisplay,
                     std::vector<HWC2::Layer*> aLayers) {
    for (auto layer : aLayers) {
      EXPECT_NO_HWCERROR(aDisplay->destroyLayer(layer));
    }
  }

  void SetLayerProperties(HWC2::Display* aDisplay, HWC2::Layer* aLayer,
                          Hwc2TestLayers* aTestLayers, bool* aOutSkip) {
    *aOutSkip = true;

    if (!aTestLayers->Contains(aLayer)) {
      return;
    }

    HWC2::Composition composition = aTestLayers->GetComposition(aLayer);

    int slot = -1;
    android::sp<android::GraphicBuffer> buffer;
    android::sp<android::Fence> acquireFence = android::Fence::NO_FENCE;

    /* If the device cannot support a buffer format, then do not continue */
    if ((composition == HWC2::Composition::Device ||
         composition == HWC2::Composition::Cursor) &&
        aTestLayers->GetBuffer(aLayer, slot, buffer, acquireFence) < 0) {
      return;
    }

    HWC2::Error err;
    EXPECT_NO_HWCERROR(err = aLayer->setCompositionType(composition));
    if (err == HWC2::Error::Unsupported) {
      EXPECT_TRUE(composition != HWC2::Composition::Client &&
                  composition != HWC2::Composition::Device);
    }

    EXPECT_NO_HWCERROR(aLayer->setBuffer(slot, buffer, acquireFence));

    EXPECT_NO_HWCERROR(aLayer->setBlendMode(aTestLayers->GetBlendMode(aLayer)));
    EXPECT_NO_HWCERROR(aLayer->setColor(aTestLayers->GetColor(aLayer)));

    if (composition == HWC2::Composition::Cursor) {
      const android::Rect cursor = aTestLayers->GetCursorPosition(aLayer);
      EXPECT_NO_FATAL_FAILURE(
          aLayer->setCursorPosition(cursor.left, cursor.top));
    }

    EXPECT_NO_HWCERROR(aLayer->setDataspace(aTestLayers->GetDataspace(aLayer)));
    EXPECT_NO_HWCERROR(
        aLayer->setDisplayFrame(aTestLayers->GetDisplayFrame(aLayer)));
    EXPECT_NO_HWCERROR(
        aLayer->setPlaneAlpha(aTestLayers->GetPlaneAlpha(aLayer)));
    EXPECT_NO_HWCERROR(
        aLayer->setSourceCrop(aTestLayers->GetSourceCrop(aLayer)));
    EXPECT_NO_HWCERROR(
        aLayer->setSurfaceDamage(aTestLayers->GetSurfaceDamage(aLayer)));
    EXPECT_NO_HWCERROR(aLayer->setTransform(aTestLayers->GetTransform(aLayer)));
    EXPECT_NO_HWCERROR(
        aLayer->setVisibleRegion(aTestLayers->GetVisibleRegion(aLayer)));
    EXPECT_NO_HWCERROR(aLayer->setZOrder(aTestLayers->GetZOrder(aLayer)));

    *aOutSkip = false;
  }

  void SetLayerProperties(HWC2::Display* aDisplay,
                          const std::vector<HWC2::Layer*>& aLayers,
                          Hwc2TestLayers* aTestLayers, bool* aOutSkip) {
    for (auto layer : aLayers) {
      EXPECT_NO_FATAL_FAILURE(
          SetLayerProperties(aDisplay, layer, aTestLayers, aOutSkip));
      if (*aOutSkip) return;
    }
  }

  void DisplayLayers(Hwc2TestCoverage aCoverage, size_t aLayerCnt,
                     TestDisplayLayersFunction aFunction) {
    for (auto displayId : mDisplayIds) {
      auto display = mHwc->getDisplayById(displayId);

      ASSERT_NO_HWCERROR(display->setPowerMode(HWC2::PowerMode::On));
      auto configs = display->getConfigs();

      for (auto config : configs) {
        ASSERT_NO_HWCERROR(display->setActiveConfig(config));

        Area displayArea = GetActiveDisplayArea(config);
        std::vector<HWC2::Layer*> layers;

        ASSERT_NO_FATAL_FAILURE(CreateLayers(display, aLayerCnt, &layers));
        Hwc2TestLayers testLayers(layers, aCoverage, displayArea);

        do {
          bool skip;

          ASSERT_NO_FATAL_FAILURE(
              SetLayerProperties(display, layers, &testLayers, &skip));
          if (!skip) {
            EXPECT_NO_FATAL_FAILURE(
                aFunction(this, display, layers, &testLayers));
          }

        } while (testLayers.Advance());

        ASSERT_NO_FATAL_FAILURE(DestroyLayers(display, std::move(layers)));
      }

      ASSERT_NO_HWCERROR(display->setPowerMode(HWC2::PowerMode::Off));
    }
  }

  void HandleRequests(HWC2::Display* aDisplay,
                      const std::vector<HWC2::Layer*>& aLayers,
                      uint32_t numRequests,
                      std::set<HWC2::Layer*>* outClearLayers = nullptr,
                      bool* outFlipClientTarget = nullptr) {
    HWC2::DisplayRequest displayRequest;
    std::unordered_map<HWC2::Layer*, HWC2::LayerRequest> layerRequests;

    aDisplay->getRequests(&displayRequest, &layerRequests);

    EXPECT_EQ(numRequests, layerRequests.size())
        << "validate returned " << numRequests
        << " requests and get display requests returned "
        << layerRequests.size() << " requests";

    for (auto& [requestedLayer, request] : layerRequests) {
      if (outClearLayers && request == HWC2::LayerRequest::ClearClientTarget) {
        outClearLayers->insert(requestedLayer);
      }
    }

    if (outFlipClientTarget) {
      *outFlipClientTarget =
          static_cast<int>(displayRequest) &
          static_cast<int>(HWC2::DisplayRequest::FlipClientTarget);
    }
  }

  void HandleCompositionChanges(
      HWC2::Display* display, const Hwc2TestLayers& testLayers,
      const std::vector<HWC2::Layer*>& layers, uint32_t numTypes,
      std::set<HWC2::Layer*>* outClientLayers = nullptr) {
    std::unordered_map<HWC2::Layer*, HWC2::Composition> changedTypes;

    ASSERT_NO_HWCERROR(display->getChangedCompositionTypes(&changedTypes));

    EXPECT_EQ(numTypes, changedTypes.size())
        << "validate returned " << numTypes
        << " types and get changed composition types"
           " returned "
        << changedTypes.size() << " types";

    if (gPrintValidateResult) {
      printf("%d: ", numTypes);
      for (auto& layer : layers) {
        HWC2::Composition requestedType = testLayers.GetComposition(layer);
        float alpha = testLayers.GetPlaneAlpha(layer);

        if (changedTypes.count(layer) == 0) {
          printf("%1.1f-%s ", alpha, to_string(requestedType).c_str());
        } else {
          HWC2::Composition returnedType = changedTypes[layer];
          printf("%1.1f-%s->%s ", alpha, to_string(requestedType).c_str(),
                 to_string(returnedType).c_str());
        }
      }
      printf("\n");
    }

    for (auto& [layer, returnedType] : changedTypes) {
      HWC2::Composition requestedType = testLayers.GetComposition(layer);

      EXPECT_NE(returnedType, HWC2::Composition::Invalid)
          << "get changed"
             " composition types returned invalid composition";

      switch (requestedType) {
        case HWC2::Composition::Client:
          EXPECT_TRUE(false) << to_string(returnedType) << " cannot be changed";
          break;
        case HWC2::Composition::Device:
        case HWC2::Composition::SolidColor:
          EXPECT_EQ(returnedType, HWC2::Composition::Client)
              << "composition of type " << to_string(requestedType)
              << " can only be changed to "
              << to_string(HWC2::Composition::Client);
          break;
        case HWC2::Composition::Cursor:
        case HWC2::Composition::Sideband:
          EXPECT_TRUE(returnedType == HWC2::Composition::Client ||
                      returnedType == HWC2::Composition::Device)
              << "composition of type " << to_string(requestedType)
              << " can only be changed to "
              << to_string(HWC2::Composition::Client) << " or "
              << to_string(HWC2::Composition::Device);
          break;
        default:
          EXPECT_TRUE(false) << "unknown type " << to_string(requestedType);
          break;
      }

      if (outClientLayers)
        if (returnedType == HWC2::Composition::Client) {
          outClientLayers->insert(layer);
        }
    }

    if (outClientLayers) {
      for (auto layer : layers) {
        if (testLayers.GetComposition(layer) == HWC2::Composition::Client) {
          outClientLayers->insert(layer);
        }
      }
    }
  }

  void SetClientTarget(HWC2::Display* display,
                       Hwc2TestClientTarget* testClientTarget,
                       const Hwc2TestLayers& testLayers,
                       const std::set<HWC2::Layer*>& clientLayers,
                       const std::set<HWC2::Layer*>& clearLayers,
                       bool flipClientTarget, const Area& displayArea) {
    android::Hwc2::Dataspace dataspace = android::Hwc2::Dataspace::UNKNOWN;
    android::Region damage = {};
    int slot;
    android::sp<android::GraphicBuffer> buffer;
    android::sp<android::Fence> acquireFence;

    ASSERT_EQ(testClientTarget->GetBuffer(testLayers, clientLayers, clearLayers,
                                          flipClientTarget, displayArea, slot,
                                          buffer, acquireFence),
              0);
    EXPECT_NO_HWCERROR(display->setClientTarget(slot, buffer, acquireFence,
                                                dataspace, damage));
  }

  void CloseFences(HWC2::Display* display,
                   android::sp<android::Fence> presentFence) {
    const int msWait = 3000;

    if (presentFence->isValid()) {
      ASSERT_GE(presentFence->wait(msWait), 0);
      presentFence = nullptr;
    }

    std::unordered_map<HWC2::Layer*, android::sp<android::Fence>> fences;
    display->getReleaseFences(&fences);

    for (auto& [layer, fence] : fences) {
      if (fence->isValid()) {
        EXPECT_GE(fence->wait(msWait), 0);
        fence = nullptr;
      }
    }
  }

  void PresentDisplays(
      size_t layerCnt, Hwc2TestCoverage coverage,
      const std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage>&
          coverageExceptions,
      bool optimize) {
    for (auto displayId : mDisplayIds) {
      auto display = mHwc->getDisplayById(displayId);

      ASSERT_NO_HWCERROR(display->setPowerMode(HWC2::PowerMode::On));
      ASSERT_NO_HWCERROR(display->setVsyncEnabled(HWC2::Vsync::Enable));
      auto configs = display->getConfigs();

      for (auto config : configs) {
        ASSERT_NO_HWCERROR(display->setActiveConfig(config));

        Area displayArea = GetActiveDisplayArea(config);
        std::vector<HWC2::Layer*> layers;

        ASSERT_NO_FATAL_FAILURE(CreateLayers(display, layerCnt, &layers));
        Hwc2TestLayers testLayers(layers, coverage, displayArea,
                                  coverageExceptions);

        if (optimize && !testLayers.OptimizeLayouts()) {
          continue;
        }

        std::set<HWC2::Layer*> clientLayers;
        std::set<HWC2::Layer*> clearLayers;
        Hwc2TestClientTarget testClientTarget;

        do {
          uint32_t numTypes, numRequests;
          bool skip;
          bool flipClientTarget;

          ASSERT_NO_FATAL_FAILURE(
              SetLayerProperties(display, layers, &testLayers, &skip));
          if (skip) continue;

          auto error = display->validate(&numTypes, &numRequests);

          if (error == HWC2::Error::HasChanges) {
            EXPECT_LE(numTypes, static_cast<uint32_t>(layers.size()))
                << "wrong number of requests";
          }

          ASSERT_NO_FATAL_FAILURE(HandleCompositionChanges(
              display, testLayers, layers, numTypes, &clientLayers));

          EXPECT_NO_FATAL_FAILURE(HandleRequests(
              display, layers, numRequests, &clearLayers, &flipClientTarget));

          ASSERT_NO_FATAL_FAILURE(SetClientTarget(
              display, &testClientTarget, testLayers, clientLayers, clearLayers,
              flipClientTarget, displayArea));

          ASSERT_NO_HWCERROR(display->acceptChanges());

          ASSERT_NO_FATAL_FAILURE(WaitForVsync());

          android::sp<android::Fence> presentFence;
          ASSERT_NO_HWCERROR(display->present(&presentFence));

          ASSERT_NO_FATAL_FAILURE(
              CloseFences(display, std::move(presentFence)));

        } while (testLayers.Advance());

        ASSERT_NO_FATAL_FAILURE(DestroyLayers(display, std::move(layers)));
      }

      ASSERT_NO_HWCERROR(display->setVsyncEnabled(HWC2::Vsync::Disable));
      ASSERT_NO_HWCERROR(display->setPowerMode(HWC2::PowerMode::Off));
    }
  }

 protected:
  std::mutex mHotplugMutex;
  std::condition_variable mHotplugCv;
  hwc2_display_t mVsyncDisplay = 0;
  int64_t mVsyncTimestamp = 0;

  std::mutex mVsyncMutex;
  std::condition_variable mVsyncCv;

  std::unique_ptr<HWC2::Device> mHwc = nullptr;
  HWC2::Display* mPrimaryDisplay = nullptr;

  std::unordered_set<hwc2_display_t> mDisplayIds;
};

TEST_F(Hwc2Test, GET_CAPABILITIES) {
  auto capabilities = mHwc->getCapabilities();

  EXPECT_EQ(capabilities.count(HWC2::Capability::Invalid), 0);
}

TEST_F(Hwc2Test, CONNECT_TO_PRIMARYDISPLAY) {
  ASSERT_NE(mHwc, nullptr);

  std::shared_ptr<const HWC2::Display::Config> hwcConfig;

  auto primaryDisplay = EnsurePrimaryDisplay();
  ASSERT_TRUE(primaryDisplay);

  auto error = primaryDisplay->getActiveConfig(&hwcConfig);
  ASSERT_EQ(error, HWC2::Error::None);
  ASSERT_TRUE(hwcConfig.get());

  printf("layer size: %d x %d\n", hwcConfig->getWidth(),
         hwcConfig->getHeight());
}

TEST_F(Hwc2Test, CREATE_DESTROY_LAYER) {
  for (auto displayId : mDisplayIds) {
    auto display = mHwc->getDisplayById(displayId);

    HWC2::Layer* layer;
    ASSERT_NO_HWCERROR(display->createLayer(&layer));
    ASSERT_NO_HWCERROR(display->destroyLayer(layer));
  }
}

TEST_F(Hwc2Test, SET_VSYNC_ENABLED_callback) {
  for (auto displayId : mDisplayIds) {
    auto display = mHwc->getDisplayById(displayId);

    ASSERT_EQ(display->setPowerMode(HWC2::PowerMode::On), HWC2::Error::None);
    ASSERT_EQ(display->setVsyncEnabled(HWC2::Vsync::Enable), HWC2::Error::None);

    hwc2_display_t receivedDisplay;
    int64_t receivedTimestamp;

    ASSERT_NO_FATAL_FAILURE(WaitForVsync(&receivedDisplay, &receivedTimestamp));

    EXPECT_EQ(receivedDisplay, displayId) << "failed to get correct display";
    EXPECT_GE(receivedTimestamp, 0) << "failed to get valid timestamp";

    ASSERT_NO_HWCERROR(display->setVsyncEnabled(HWC2::Vsync::Disable));
    ASSERT_NO_HWCERROR(display->setPowerMode(HWC2::PowerMode::Off));
  }
}

/* TESTCASE: Tests that the HWC2 can display 5 layers with default coverage. */
TEST_F(Hwc2Test, VALIDATE_DISPLAY_default_5) {
  ASSERT_NO_FATAL_FAILURE(DisplayLayers(
      Hwc2TestCoverage::Default, 5,
      [](Hwc2Test* /*aTest*/, HWC2::Display* aDisplay,
         const std::vector<HWC2::Layer*>& aLayers,
         Hwc2TestLayers* /*testLayers*/) {
        uint32_t numTypes, numRequests;

        auto error = aDisplay->validate(&numTypes, &numRequests);
        if (error == HWC2::Error::HasChanges) {
          EXPECT_LE(numTypes, static_cast<uint32_t>(aLayers.size()))
              << "wrong number of requests";
        }
      }));
}

/* TESTCASE: Tests that the HWC2 can get display requests after validating a
 * basic layer. */
TEST_F(Hwc2Test, GET_DISPLAY_REQUESTS_basic) {
  ASSERT_NO_FATAL_FAILURE(DisplayLayers(
      Hwc2TestCoverage::Basic, 1,
      [](Hwc2Test* aTest, HWC2::Display* aDisplay,
         const std::vector<HWC2::Layer*>& aLayers,
         Hwc2TestLayers* /*testLayers*/) {
        uint32_t numTypes, numRequests;

        auto error = aDisplay->validate(&numTypes, &numRequests);
        if (error == HWC2::Error::HasChanges) {
          EXPECT_LE(numTypes, static_cast<uint32_t>(aLayers.size()))
              << "wrong number of requests";
        }

        EXPECT_NO_FATAL_FAILURE(
            aTest->HandleRequests(aDisplay, aLayers, numRequests));
      }));
}

/* TESTCASE: Tests that the HWC2 can present 1 layer with complete coverage of
 * plane alpha. */
TEST_F(Hwc2Test, PRESENT_DISPLAY_layer_test_1) {
  const size_t layerCnt = 1;
  Hwc2TestCoverage coverage = Hwc2TestCoverage::Default;
  std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage> exceptions = {
      {Hwc2TestPropertyName::PlaneAlpha, Hwc2TestCoverage::Basic},
  };
  bool optimize = false;

  ASSERT_NO_FATAL_FAILURE(
      PresentDisplays(layerCnt, coverage, exceptions, optimize));
}

/* TESTCASE: Tests that the HWC2 can present 2 layer with complete coverage of
 * plane alpha. */
TEST_F(Hwc2Test, PRESENT_DISPLAY_layer_test_2) {
  const size_t layerCnt = 2;
  Hwc2TestCoverage coverage = Hwc2TestCoverage::Default;
  std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage> exceptions = {
      {Hwc2TestPropertyName::PlaneAlpha, Hwc2TestCoverage::Basic},
  };
  bool optimize = false;

  ASSERT_NO_FATAL_FAILURE(
      PresentDisplays(layerCnt, coverage, exceptions, optimize));
}

/* TESTCASE: Tests that the HWC2 can present 3 layer with complete coverage of
 * plane alpha. */
TEST_F(Hwc2Test, PRESENT_DISPLAY_layer_test_3) {
  const size_t layerCnt = 3;
  Hwc2TestCoverage coverage = Hwc2TestCoverage::Default;
  std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage> exceptions = {
      {Hwc2TestPropertyName::PlaneAlpha, Hwc2TestCoverage::Basic},
  };
  bool optimize = false;

  ASSERT_NO_FATAL_FAILURE(
      PresentDisplays(layerCnt, coverage, exceptions, optimize));
}

/* TESTCASE: Tests that the HWC2 can present 4 layer with complete coverage of
 * plane alpha. */
TEST_F(Hwc2Test, PRESENT_DISPLAY_layer_test_4) {
  const size_t layerCnt = 4;
  Hwc2TestCoverage coverage = Hwc2TestCoverage::Default;
  std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage> exceptions = {
      {Hwc2TestPropertyName::PlaneAlpha, Hwc2TestCoverage::Basic},
  };
  bool optimize = false;

  ASSERT_NO_FATAL_FAILURE(
      PresentDisplays(layerCnt, coverage, exceptions, optimize));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  while (1) {
    int option_index = 0;
    struct option long_options[] = {
        {"ShowValidateResult", 0, 0, 's'},
        {0, 0, 0, 0},
    };

    int opt = getopt_long(argc, argv, "s", long_options, &option_index);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 's':
        gPrintValidateResult = true;
        break;
    }
  }

  return RUN_ALL_TESTS();
}
