// GENERATED from src/include/render/IRenderPipeline.h by tools/port/gen_sokol_stubs.py -- do not edit.
// Default implementation of every pipeline method: report once, succeed, write no outputs.
// FSokolDevice overrides the ones that are really implemented.
virtual GENRESULT DACOM_API Startup(const char* SectionName) override { return Stub("IRenderPipeline::Startup"); }
virtual GENRESULT DACOM_API ShutdownPipeline() override { return Stub("IRenderPipeline::ShutdownPipeline"); }
virtual GENRESULT DACOM_API SetPipelineState(int Id, int Value) override { return Stub("IRenderPipeline::SetPipelineState"); }
virtual GENRESULT DACOM_API GetPipelineState(int Id, int* Out) override { return Stub("IRenderPipeline::GetPipelineState"); }
virtual GENRESULT DACOM_API GetDeviceInfo(void* Out520) override { return Stub("IRenderPipeline::GetDeviceInfo"); }
virtual GENRESULT DACOM_API QueryDeviceAbility(unsigned int Index, uint32_t* Out) override { return Stub("IRenderPipeline::QueryDeviceAbility"); }
virtual GENRESULT DACOM_API GetDisplayModeCount(unsigned int* Out) override { return Stub("IRenderPipeline::GetDisplayModeCount"); }
virtual GENRESULT DACOM_API GetDisplayModeEnum(unsigned int Index, void* Out) override { return Stub("IRenderPipeline::GetDisplayModeEnum"); }
virtual GENRESULT DACOM_API SelectMode(int* Mode, unsigned int* OutIndex) override { return Stub("IRenderPipeline::SelectMode"); }
virtual GENRESULT DACOM_API CreateBuffers(HWND Hwnd, int* Mode, int* OutMode) override { return Stub("IRenderPipeline::CreateBuffers"); }
virtual GENRESULT DACOM_API GetModeBlob(void* OutAdapter, void* OutMode10Dwords) override { return Stub("IRenderPipeline::GetModeBlob"); }
virtual GENRESULT DACOM_API DestroyGpuResources() override { return Stub("IRenderPipeline::DestroyGpuResources"); }
virtual GENRESULT DACOM_API ClearBuffers(uint32_t Flags, const void* Rects) override { return Stub("IRenderPipeline::ClearBuffers"); }
virtual GENRESULT DACOM_API SwapBuffers() override { return Stub("IRenderPipeline::SwapBuffers"); }
virtual GENRESULT DACOM_API LockBuffer(void* Out) override { return Stub("IRenderPipeline::LockBuffer"); }
virtual GENRESULT DACOM_API UnlockBuffer() override { return Stub("IRenderPipeline::UnlockBuffer"); }
virtual GENRESULT DACOM_API GetBufferInterface(void* Out, void* Unused) override { return Stub("IRenderPipeline::GetBufferInterface"); }
virtual GENRESULT DACOM_API GetDeviceStats(void* Out) override { return Stub("IRenderPipeline::GetDeviceStats"); }
virtual GENRESULT DACOM_API SetViewportRect(int X, int Y, int W, int H) override { return Stub("IRenderPipeline::SetViewportRect"); }
virtual GENRESULT DACOM_API GetViewportRect(int* OutX, int* OutY, int* OutW, int* OutH) override { return Stub("IRenderPipeline::GetViewportRect"); }
virtual GENRESULT DACOM_API SetDepthRange(float MinZ, float MaxZ) override { return Stub("IRenderPipeline::SetDepthRange"); }
virtual GENRESULT DACOM_API GetDepthRange(float* OutMinZ, float* OutMaxZ) override { return Stub("IRenderPipeline::GetDepthRange"); }
virtual GENRESULT DACOM_API SetWindow(HWND Hwnd, int X, int Y, int W, int H) override { return Stub("IRenderPipeline::SetWindow"); }
virtual GENRESULT DACOM_API GetWindow(HWND* OutHwnd, int* OutLeft, int* OutRight, int* OutW, int* OutH) override { return Stub("IRenderPipeline::GetWindow"); }
virtual GENRESULT DACOM_API SetWorld(const float* M12) override { return Stub("IRenderPipeline::SetWorld"); }
virtual GENRESULT DACOM_API GetWorld(float* Out12) override { return Stub("IRenderPipeline::GetWorld"); }
virtual GENRESULT DACOM_API SetView(const float* M12) override { return Stub("IRenderPipeline::SetView"); }
virtual GENRESULT DACOM_API GetView(float* Out12) override { return Stub("IRenderPipeline::GetView"); }
virtual GENRESULT DACOM_API SetModelview(const float* M12) override { return Stub("IRenderPipeline::SetModelview"); }
virtual GENRESULT DACOM_API GetModelview(float* Out12) override { return Stub("IRenderPipeline::GetModelview"); }
virtual GENRESULT DACOM_API SetProjection(const float* M16) override { return Stub("IRenderPipeline::SetProjection"); }
virtual GENRESULT DACOM_API GetProjection(float* Out16) override { return Stub("IRenderPipeline::GetProjection"); }
virtual GENRESULT DACOM_API SetLookat(float EyeX, float EyeY, float EyeZ, float AtX, float AtY, float AtZ, float UpX, float UpY, float UpZ) override { return Stub("IRenderPipeline::SetLookat"); }
virtual GENRESULT DACOM_API SetOrtho(float Left, float Right, float Bottom, float Top, float NearPlane, float FarPlane) override { return Stub("IRenderPipeline::SetOrtho"); }
virtual GENRESULT DACOM_API SetPerspective(float FovDeg, float Aspect, float NearPlane, float FarPlane) override { return Stub("IRenderPipeline::SetPerspective"); }
virtual GENRESULT DACOM_API SetLight(void* Handle, const D3DLIGHT8* Light) override { return Stub("IRenderPipeline::SetLight"); }
virtual GENRESULT DACOM_API DestroyLight(void* Handle) override { return Stub("IRenderPipeline::DestroyLight"); }
virtual GENRESULT DACOM_API GetLight(void* Handle, D3DLIGHT8* OutLight) override { return Stub("IRenderPipeline::GetLight"); }
virtual GENRESULT DACOM_API SetLightEnable(void* Handle, int Enable) override { return Stub("IRenderPipeline::SetLightEnable"); }
virtual GENRESULT DACOM_API GetLightEnable(void* Handle, int* OutEnable) override { return Stub("IRenderPipeline::GetLightEnable"); }
virtual GENRESULT DACOM_API SetMaterial(const D3DMATERIAL8* Material) override { return Stub("IRenderPipeline::SetMaterial"); }
virtual GENRESULT DACOM_API GetMaterial(D3DMATERIAL8* Material) override { return Stub("IRenderPipeline::GetMaterial"); }
virtual GENRESULT DACOM_API CreateTexture(int Width, int Height, unsigned int* Format, unsigned int Levels, unsigned int Flags, void** OutHandle) override { return Stub("IRenderPipeline::CreateTexture"); }
virtual GENRESULT DACOM_API DestroyTexture(void* Handle) override { return Stub("IRenderPipeline::DestroyTexture"); }
virtual GENRESULT DACOM_API IsTexture(void* Handle) override { return Stub("IRenderPipeline::IsTexture"); }
virtual GENRESULT DACOM_API LockTexture(void* Handle, int Level, void* Out) override { return Stub("IRenderPipeline::LockTexture"); }
virtual GENRESULT DACOM_API UnlockTexture(void* Handle, int Level) override { return Stub("IRenderPipeline::UnlockTexture"); }
virtual GENRESULT DACOM_API GetTextureFormat(void* Handle, int* OutFormat) override { return Stub("IRenderPipeline::GetTextureFormat"); }
virtual GENRESULT DACOM_API GetTextureDim(void* Handle, unsigned int* OutW, unsigned int* OutH, int* OutLevels) override { return Stub("IRenderPipeline::GetTextureDim"); }
virtual GENRESULT DACOM_API GetTextureInterface(void* Handle, void* UnusedA, void* UnusedB) override { return Stub("IRenderPipeline::GetTextureInterface"); }
virtual GENRESULT DACOM_API SetTextureLevelData(void* Handle, int Level, int SrcW, int SrcH, unsigned int Pitch, unsigned int* SrcFormat, const void* Pixels, void* A, void* B) override { return Stub("IRenderPipeline::SetTextureLevelData"); }
virtual GENRESULT DACOM_API BlitTexture(void* DstHandle, int DstLevel, void* SrcHandle, int SrcLevel) override { return Stub("IRenderPipeline::BlitTexture"); }
virtual GENRESULT DACOM_API SetRenderTarget(void* Handle, void* UnusedA, void* UnusedB) override { return Stub("IRenderPipeline::SetRenderTarget"); }
virtual GENRESULT DACOM_API GetRenderTarget(void* Handle) override { return Stub("IRenderPipeline::GetRenderTarget"); }
virtual GENRESULT DACOM_API BeginScene() override { return Stub("IRenderPipeline::BeginScene"); }
virtual GENRESULT DACOM_API EndScene() override { return Stub("IRenderPipeline::EndScene"); }
virtual GENRESULT DACOM_API ResetRenderStatesToDefaults() override { return Stub("IRenderPipeline::ResetRenderStatesToDefaults"); }
virtual GENRESULT DACOM_API SetRenderState(D3DRENDERSTATETYPE State, int Value) override { return Stub("IRenderPipeline::SetRenderState"); }
virtual GENRESULT DACOM_API GetRenderState(D3DRENDERSTATETYPE State, int* Out) override { return Stub("IRenderPipeline::GetRenderState"); }
virtual GENRESULT DACOM_API SetTextureStageState(int Stage, int Type, int Value) override { return Stub("IRenderPipeline::SetTextureStageState"); }
virtual GENRESULT DACOM_API GetTextureStageState(int Stage, int Type, int* Out) override { return Stub("IRenderPipeline::GetTextureStageState"); }
virtual GENRESULT DACOM_API SetTextureStageTransform(int Stage, const float* Matrix16) override { return Stub("IRenderPipeline::SetTextureStageTransform"); }
virtual GENRESULT DACOM_API GetTextureStageTransform(int Stage, float* OutMatrix16) override { return Stub("IRenderPipeline::GetTextureStageTransform"); }
virtual GENRESULT DACOM_API SetTextureStageTexture(int Stage, void* Handle) override { return Stub("IRenderPipeline::SetTextureStageTexture"); }
virtual GENRESULT DACOM_API GetTextureStageTexture(int Stage, void** OutHandle) override { return Stub("IRenderPipeline::GetTextureStageTexture"); }
virtual GENRESULT DACOM_API VerifyState() override { return Stub("IRenderPipeline::VerifyState"); }
virtual GENRESULT DACOM_API DrawPrimitive(uint32_t Prim, unsigned int Fvf, const void* Verts, unsigned int Count, void* Unused) override { return Stub("IRenderPipeline::DrawPrimitive"); }
virtual GENRESULT DACOM_API DrawIndexedPrimitiveUp(uint32_t Prim, unsigned int Fvf, const void* Verts, unsigned int VertCount, const void* Indices, unsigned int IndexCount, uint32_t Unused) override { return Stub("IRenderPipeline::DrawIndexedPrimitiveUp"); }
virtual GENRESULT DACOM_API DrawPrimitiveVb(uint32_t Prim, void* Vb, unsigned int StartVertex, unsigned int Count, void* Unused) override { return Stub("IRenderPipeline::DrawPrimitiveVb"); }
virtual GENRESULT DACOM_API DrawIndexedPrimitiveVb(uint32_t Prim, void* Vb, unsigned int BaseVertex, unsigned int NumVerts, const void* Indices, unsigned int IndexCount, uint32_t Unused) override { return Stub("IRenderPipeline::DrawIndexedPrimitiveVb"); }
virtual GENRESULT DACOM_API AddLight(void* Handle) override { return Stub("IRenderPipeline::AddLight"); }
virtual GENRESULT DACOM_API RemoveLight(void* Handle) override { return Stub("IRenderPipeline::RemoveLight"); }
virtual GENRESULT DACOM_API UpdateLight(void* Handle) override { return Stub("IRenderPipeline::UpdateLight"); }
virtual GENRESULT DACOM_API SetWorldN(int WorldIndex, const float* M12) override { return Stub("IRenderPipeline::SetWorldN"); }
virtual GENRESULT DACOM_API VbMgrInit(void* A, void* B, void* C, void* D) override { return Stub("IVertexBufferManager::VbMgrInit"); }
virtual GENRESULT DACOM_API VbMgrReleaseAll() override { return Stub("IVertexBufferManager::VbMgrReleaseAll"); }
virtual GENRESULT DACOM_API LockVertexMemory(unsigned int Fvf, unsigned int Count, FVertexMemoryLock* Out) override { return Stub("IVertexBufferManager::LockVertexMemory"); }
virtual GENRESULT DACOM_API UnlockVertexMemory(FVertexMemoryLock* Out) override { return Stub("IVertexBufferManager::UnlockVertexMemory"); }
virtual GENRESULT DACOM_API UnsupportedManagerOperation() override { return Stub("IVertexBufferManager::UnsupportedManagerOperation"); }
virtual GENRESULT DACOM_API CopyVertexData(void* Dest, unsigned int Fvf, const FRpVertexSrcDesc* Source, unsigned int StartVertex, unsigned int Count) override { return Stub("IVertexBufferManager::CopyVertexData"); }
virtual GENRESULT DACOM_API DrawIndexedPrimitive(uint32_t Prim, unsigned int MinIndex, unsigned int NumVertices, unsigned int StartIndex, unsigned int Count) override { return Stub("IRPDraw::DrawIndexedPrimitive"); }
virtual GENRESULT DACOM_API CreateIb(unsigned int Count, unsigned int** Out, unsigned int Flags) override { return Stub("IRPIndexBuffer::CreateIb"); }
virtual GENRESULT DACOM_API DestroyIb(void** Handle) override { return Stub("IRPIndexBuffer::DestroyIb"); }
virtual GENRESULT DACOM_API ResizeIb(void* Handle, unsigned int Count) override { return Stub("IRPIndexBuffer::ResizeIb"); }
virtual GENRESULT DACOM_API CopyIndices(void* Handle, unsigned int* OutOffset, const unsigned short* SrcData, unsigned int Count) override { return Stub("IRPIndexBuffer::CopyIndices"); }
virtual GENRESULT DACOM_API LockIb(void* Handle, unsigned int* Offset, void** Out, unsigned int Count) override { return Stub("IRPIndexBuffer::LockIb"); }
virtual GENRESULT DACOM_API UnlockIb(void* Handle) override { return Stub("IRPIndexBuffer::UnlockIb"); }
virtual GENRESULT DACOM_API SelectIb(void* Handle, unsigned int BaseVertexIndex, unsigned int Unused3, unsigned int Unused4) override { return Stub("IRPIndexBuffer::SelectIb"); }
virtual GENRESULT DACOM_API GetIbCapacity(void* Handle, unsigned int* OutCapacity) override { return Stub("IRPIndexBuffer::GetIbCapacity"); }
virtual GENRESULT DACOM_API IsIbValid(void* Handle) override { return Stub("IRPIndexBuffer::IsIbValid"); }
virtual GENRESULT DACOM_API CreateVb(unsigned int Fvf, unsigned int Count, unsigned int** Out, unsigned int Flags) override { return Stub("IRPVertexBuffer::CreateVb"); }
virtual GENRESULT DACOM_API DestroyVb(void** Handle) override { return Stub("IRPVertexBuffer::DestroyVb"); }
virtual GENRESULT DACOM_API ResizeVb(void* Handle, unsigned int Fvf, unsigned int Count) override { return Stub("IRPVertexBuffer::ResizeVb"); }
virtual GENRESULT DACOM_API CopyVertices(void* Handle, unsigned int* OutOffset, const FRpVertexSrcDesc* SrcData, unsigned int StartVertex, unsigned int Count) override { return Stub("IRPVertexBuffer::CopyVertices"); }
virtual GENRESULT DACOM_API LockVb(void* Handle, unsigned int* Offset, void** Out, unsigned int Count) override { return Stub("IRPVertexBuffer::LockVb"); }
virtual GENRESULT DACOM_API UnlockVb(void* Handle) override { return Stub("IRPVertexBuffer::UnlockVb"); }
virtual GENRESULT DACOM_API UnsupportedVertexBufferOperation(void* Handle) override { return Stub("IRPVertexBuffer::UnsupportedVertexBufferOperation"); }
virtual GENRESULT DACOM_API SelectVb(void* Handle) override { return Stub("IRPVertexBuffer::SelectVb"); }
virtual GENRESULT DACOM_API GetVbInfo(void* Handle, unsigned int* OutFvf, unsigned int* OutCount) override { return Stub("IRPVertexBuffer::GetVbInfo"); }
virtual GENRESULT DACOM_API IsVbValid(void* Handle) override { return Stub("IRPVertexBuffer::IsVbValid"); }
virtual GENRESULT DACOM_API SetGammaFunction(unsigned int ChannelMask, float Gamma, float Brightness, float Contrast, float Bias) override { return Stub("IGammaControl::SetGammaFunction"); }
virtual GENRESULT DACOM_API SetGammaRamp(unsigned int ChannelMask, const unsigned short* RampData) override { return Stub("IGammaControl::SetGammaRamp"); }
virtual GENRESULT DACOM_API GetGammaRamp(unsigned int Channel, unsigned short* OutRamp) override { return Stub("IGammaControl::GetGammaRamp"); }
virtual GENRESULT DACOM_API SetGammaEnable(unsigned char Enable) override { return Stub("IGammaControl::SetGammaEnable"); }
virtual GENRESULT DACOM_API GetGammaEnable() override { return Stub("IGammaControl::GetGammaEnable"); }
virtual GENRESULT DACOM_API PrintScreen(void* ParentFileSystem, const char* FileName) override { return Stub("IRPTexture::PrintScreen"); }
virtual GENRESULT DACOM_API LoadTexture(IFileSystem* FileSystem, const char* FileName, void** OutHandle) override { return Stub("IRPTexture::LoadTexture"); }
virtual GENRESULT DACOM_API LoadSurfaceData(void* Handle, int Level, unsigned int SrcSize, const void* SrcData) override { return Stub("IRPTexture::LoadSurfaceData"); }
virtual GENRESULT DACOM_API GetImageInfo(_D3DXIMAGE_INFO* Info, unsigned int SrcSize, const void* SrcData) override { return Stub("IRPTexture::GetImageInfo"); }
virtual GENRESULT DACOM_API LoadCubemap(IFileSystem* FileSystem, const char* FileName, void** OutHandle) override { return Stub("IRPTexture::LoadCubemap"); }
