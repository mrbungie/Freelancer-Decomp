#include "fl/text.h"

namespace fl { namespace text
{
    namespace { IRasterizer* G_Rasterizer = nullptr; }
    void SetRasterizer(IRasterizer* r) { G_Rasterizer = r; }
    IRasterizer* GetRasterizer() { return G_Rasterizer; }
}}
