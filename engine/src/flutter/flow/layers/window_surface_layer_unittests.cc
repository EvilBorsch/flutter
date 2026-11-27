// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/window_surface_layer.h"

#include "flutter/flow/layers/opacity_layer.h"
#include "flutter/flow/layers/texture_layer.h"
#include "flutter/flow/testing/mock_embedder.h"
#include "gtest/gtest.h"
#include "third_party/skia/include/core/SkColorSpace.h"

namespace flutter::testing {
namespace {

class LayeredViewEmbedder : public MockViewEmbedder {
 public:
  bool DenialLayeredComposition() const override { return true; }

  int64_t DenialRegisterCut(std::unique_ptr<EmbeddedViewParams>,
                            DenialCutEntry entry) override {
    cuts.push_back(entry.kind);
    return cuts.size() - 1;
  }

  std::vector<DenialCutEntry::Kind> cuts;
};

TEST(WindowSurfaceLayerTest, ComposedWindowFadesAsAGroup) {
  const auto bounds = DlRect::MakeWH(120, 100);
  auto window = std::make_shared<WindowSurfaceLayer>(
      DlWindowSurfaceFilter::Style{.bounds = bounds, .content_bounds = bounds},
      nullptr, -1, DlRect(), DlMatrix(), DlImageSampling::kLinear, 1.0f);
  window->Add(std::make_shared<TextureLayer>(DlPoint(), bounds.GetSize(), 1,
                                             false, DlImageSampling::kLinear));
  window->Add(std::make_shared<TextureLayer>(DlPoint(20, 20), DlSize(60, 50), 2,
                                             false, DlImageSampling::kLinear));

  using Kind = DenialCutEntry::Kind;
  LayeredViewEmbedder embedder;
  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(kGiantRect, DlMatrix());
  FixedRefreshRateStopwatch stopwatch;
  PrerollContext preroll_context{
      .view_embedder = &embedder,
      .state_stack = state_stack,
      .raster_time = stopwatch,
      .ui_time = stopwatch,
  };
  window->Preroll(&preroll_context);
  EXPECT_EQ(embedder.cuts, (std::vector{Kind::kTexture, Kind::kTexture}));

  embedder.cuts.clear();
  OpacityLayer fade(128, DlPoint());
  fade.Add(window);
  fade.Preroll(&preroll_context);
  EXPECT_EQ(embedder.cuts, (std::vector{Kind::kMaterialBegin, Kind::kTexture,
                                        Kind::kTexture, Kind::kMaterialEnd}));

  embedder.cuts.clear();
  auto offset = state_stack.save();
  offset.translate(0.25f, 0);
  window->Preroll(&preroll_context);
  EXPECT_EQ(embedder.cuts, (std::vector{Kind::kMaterialBegin, Kind::kTexture,
                                        Kind::kTexture, Kind::kMaterialEnd}));
}

}  // namespace
}  // namespace flutter::testing
