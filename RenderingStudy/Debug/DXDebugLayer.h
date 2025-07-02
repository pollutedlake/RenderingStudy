#pragma once

#include <Support/WinInclude.h>
#include <Support/ComPointer.h>

class DXDebugLayer
{
public:
	bool Init();
	void Shutdown();

private:
#ifdef _DEBUG
	ComPointer<ID3D12Debug3> m_d3d12Debug;	// Windows 10, OS 빌드 19045에서는 ID3D12Debug <= 3만 지원함.
	ComPointer<IDXGIDebug1> m_dxgiDebug;
#endif

	// Singleton
public:
	DXDebugLayer(const DXDebugLayer&) = delete;
	DXDebugLayer& operator=(const DXDebugLayer&) = delete;

	inline static DXDebugLayer& Get()
	{
		static DXDebugLayer instance;
		return instance;
	}
private:
	DXDebugLayer() = default;
};