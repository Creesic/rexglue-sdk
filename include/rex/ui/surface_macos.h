#pragma once
#include <rex/ui/surface.h>

namespace rex::ui {

class MetalLayerSurface final : public Surface {
 public:
  explicit MetalLayerSurface(void* layer) : layer_(layer) {}

  TypeIndex GetType() const override { return kTypeIndex_MetalLayer; }
  void* layer() const { return layer_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  void* layer_;
};

}  // namespace rex::ui
