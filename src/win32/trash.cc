#include <wrl/implements.h>

#include <shellapi.h>
#include <shobjidl.h>

#include "../trash.h"

using namespace Microsoft::WRL;

static struct { void operator << (HRESULT hr) { if (FAILED(hr)) throw hr; } } HR;

struct CoAutoInitialize sealed
{
    explicit CoAutoInitialize(COINIT dwCoInit)
    {
        HR << ::CoInitializeEx(nullptr, dwCoInit);
    }
    ~CoAutoInitialize() noexcept
    {
        ::CoUninitialize();
    }
    CoAutoInitialize(CoAutoInitialize const &) = delete;
    CoAutoInitialize & operator = (CoAutoInitialize const &) = delete;
};

struct RecycleFileOperationProgressSink : RuntimeClass<RuntimeClassFlags<ClassicCom>, IFileOperationProgressSink>
{
    IFACEMETHODIMP StartOperations() override
    {
        return S_OK;
    }

    IFACEMETHODIMP FinishOperations(HRESULT) override
    {
        return S_OK;
    }

    IFACEMETHODIMP PreRenameItem(DWORD, IShellItem *, LPCWSTR) override { return E_NOTIMPL; }
    IFACEMETHODIMP PostRenameItem(DWORD, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override { return E_NOTIMPL; }
    IFACEMETHODIMP PreMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return E_NOTIMPL; }
    IFACEMETHODIMP PostMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override { return E_NOTIMPL; }
    IFACEMETHODIMP PreCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return E_NOTIMPL; }
    IFACEMETHODIMP PostCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override { return E_NOTIMPL; }

    IFACEMETHODIMP PreDeleteItem(DWORD dwFlags, IShellItem *) override
    {
        return (dwFlags & TSF_DELETE_RECYCLE_IF_POSSIBLE) ? S_OK : E_ABORT;
    }

    IFACEMETHODIMP PostDeleteItem(DWORD, IShellItem *, HRESULT, IShellItem *) override
    {
        return S_OK;
    }

    IFACEMETHODIMP PreNewItem(DWORD, IShellItem *, LPCWSTR) override { return E_NOTIMPL; }
    IFACEMETHODIMP PostNewItem(DWORD, IShellItem *, LPCWSTR, LPCWSTR, DWORD, HRESULT, IShellItem *) override { return E_NOTIMPL; }

    IFACEMETHODIMP UpdateProgress(UINT, UINT) override
    {
        return S_OK;
    }

    IFACEMETHODIMP ResetTimer() override
    {
        return S_OK;
    }

    IFACEMETHODIMP PauseTimer() override
    {
        return S_OK;
    }

    IFACEMETHODIMP ResumeTimer() override
    {
        return S_OK;
    }
};

static inline auto Recycle(LPCWSTR pszPath)
{
    CoAutoInitialize coinit(COINIT_APARTMENTTHREADED);

    ComPtr<IShellItem> item;
    HR << ::SHCreateItemFromParsingName(pszPath, nullptr, IID_PPV_ARGS(item.GetAddressOf()));

    ComPtr<IFileOperation> op;
    HR << ::CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(op.GetAddressOf()));
    HR << op->SetOperationFlags(FOF_NO_UI | FOFX_RECYCLEONDELETE);
    HR << op->DeleteItem(item.Get(), Make<RecycleFileOperationProgressSink>().Get());
    HR << op->PerformOperations();
}

bool az::fs::trash(const az::fs::path &path)
{
    az::ec::error_code ec;
    if (!trash(path, ec))
        throw az::fs::filesystem_error("trash", path, ec);
    return true;
}

bool az::fs::trash(const az::fs::path &path, az::ec::error_code &ec) noexcept
try
{
    Recycle(az::fs::absolute(path).c_str());
    ec.clear();
    return true;
}
catch (HRESULT hr)
{
    ec.assign(hr, az::ec::system_category());
    return false;
}
