template<typename T>
inline void Device::executeCopyCommandList(const T& function)
{
    auto& copyCommandList = getCopyCommandList();
    copyCommandList.open();
    function(copyCommandList.getUnderlyingCommandList());
    copyCommandList.close();
    uint64_t fenceValue = ++m_fenceValue;
    m_copyQueue.executeCommandList(copyCommandList);
    m_copyQueue.signal(fenceValue);
    HRESULT hr = m_copyQueue.getFence()->SetEventOnCompletion(fenceValue, nullptr);
    assert(SUCCEEDED(hr));
}