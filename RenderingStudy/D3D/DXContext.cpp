#include "DXContext.h"

/*
*	GPU
*	<--
*	CommandQueue (WorkSet, WorkSet, Sync)
*	<--
*	CPU WorkSet(Setup, Setup, ChangeState, Setup, Draw) == CommandList
*
*/

D3D_FEATURE_LEVEL featureLevels[] = {
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0
};

bool DXContext::Init()
{
	for(auto featureLevel : featureLevels)
	{
		HRESULT hardwareResult = D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&m_device));
		if (FAILED(hardwareResult))
		{
			ComPointer<IDXGIFactory6> mdxgiFactory;
			if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory))))
			{
				ComPointer<IDXGIAdapter1> pWarpAdapter;
				for (UINT adapterIdx = 0; mdxgiFactory->EnumAdapters1(adapterIdx, &pWarpAdapter) != DXGI_ERROR_NOT_FOUND; ++adapterIdx)
				{
					DXGI_ADAPTER_DESC1 desc;
					pWarpAdapter->GetDesc1(&desc);

					// 소프트웨어 어댑터는 무시
					if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
					{
						continue;
					}

					if (SUCCEEDED(D3D12CreateDevice(pWarpAdapter.Get(), featureLevel, IID_PPV_ARGS(&m_device))))
					{
						break; // 성공
					}
				}
			}
		}

		if (m_device != nullptr)
		{
			break;
		}
	}

	if (m_device == nullptr)
	{
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	if (FAILED(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue))))
	{
		return false;
	}

	if (FAILED(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
	{
		return false;
	}
	
	m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (!m_fenceEvent)
	{
		return false;
	}

	if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator))))
	{
		return false;
	}

	if (FAILED(m_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_cmdList))))
	{
		return false;
	}

	return true;
}

void DXContext::Shutdown()
{
	m_cmdList.Release();
	m_cmdAllocator.Release();
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
	}
	m_cmdQueue.Release();
	m_device.Release();
}

void DXContext::SignalAndWait()
{
	m_cmdQueue->Signal(m_fence, ++m_fenceValue);
	if (SUCCEEDED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent)))
	{
		if (WaitForSingleObject(m_fenceEvent, 20000) != WAIT_OBJECT_0)
		{
			std::exit(-1);
		}
	}
	else
	{
		std::exit(-1);
	}
}

ID3D12GraphicsCommandList7* DXContext::InitCommandList()
{
	m_cmdAllocator->Reset();
	m_cmdList->Reset(m_cmdAllocator, nullptr);
	return m_cmdList;
}

void DXContext::ExecuteCommandList()
{
	if(SUCCEEDED(m_cmdList->Close()))
	{
		ID3D12CommandList* lists[] = { m_cmdList };
		m_cmdQueue->ExecuteCommandLists(1, lists);
		SignalAndWait();
	}
}	

