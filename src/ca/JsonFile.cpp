#include "stdafx.h"

#include <codecvt>

using namespace jsoncons;
namespace fs = std::filesystem;

LPCWSTR vcsJsonFileQuery =
    L"SELECT `JsonFile`.`JsonFile`, `JsonFile`.`File`, `JsonFile`.`ElementPath`, `JsonFile`.`VerifyPath`, "
    L"`JsonFile`.`Value`, `JsonFile`.`Flags`, `JsonFile`.`Component_` FROM `JsonFile`,`Component`"
    L"WHERE `JsonFile`.`Component_`=`Component`.`Component` ORDER BY `File`, `Sequence`";

enum eJsonFileQuery { jfqId = 1, jfqFile, jfqElementPath, jfqVerifyPath, jfqValue, jfqFlags, jfqComponent };

static HRESULT UpdateJsonFile(
    __in_z LPCWSTR wzId,
    __in_z std::filesystem::path wzFile,
    __in_z LPCWSTR wzElementPath,
    __in_z LPCWSTR wzVerifyPath,
    __in_z std::wstring_view wzValue,
    __in int iFlags,
    __in_z std::wstring_view wzComponent
);
HRESULT SetJsonPathValue(std::filesystem::path const& wzFile, std::string_view sElementPath,
                      __in_z std::wstring_view wzValue, std::bitset<32> flags);

std::string ConvertToAnsi(std::wstring_view data)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    return convert.to_bytes(data.data(), data.data() + data.length());
}

std::string ConvertToAnsi(std::wstring const& data)
{
    return ConvertToAnsi(std::wstring_view{data});
}

std::string ConvertToAnsi(std::filesystem::path const& data)
{
    return ConvertToAnsi(data.native());
}


const int FLAG_DELETEVALUE = 0;
const int FLAG_SETVALUE = 1;
const int FLAG_ADDARRAYVALUE = 2;
const int FLAG_UNINSTALL = 3;
const int FLAG_PRESERVEDATE = 4;
const int FLAG_JSONPOINTER = 5;
const int FLAG_JSONBOOL = 6;
const int FLAG_JSONNUMBER = 7;
const int FLAG_JSONOBJECT = 8;
const int FLAG_JSONNULL = 9;


extern "C" UINT WINAPI JsonFile(
    __in MSIHANDLE hInstall
)
{
    HRESULT hr = WcaInitialize(hInstall, "JsonFile");

    WcaLog(LOGMSG_STANDARD, "Entered JsonFile CA");

    WcaLog(LOGMSG_STANDARD, "Created PMSIHANDLE hView");
    PMSIHANDLE hView;

    WcaLog(LOGMSG_STANDARD, "Created PMSIHANDLE hRec");
    PMSIHANDLE hRec;

    WcaLog(LOGMSG_STANDARD, "MSIHANDLE's created for JsonFile CA");

    LPWSTR sczId = nullptr;
    LPWSTR sczFile = nullptr;
    LPWSTR sczElementPath = nullptr;
    LPWSTR sczVerifyPath = nullptr;
    LPWSTR sczValue = nullptr;
    LPWSTR sczComponent = nullptr;

    INSTALLSTATE isInstalled;
    INSTALLSTATE isAction;
    bool initializedWow64{false};
    bool enabled64BitRedirection{false};
    int iFlags = 0;

    ExitOnFailure(hr, "Failed to initialize JsonFile.");

    hr = WcaInitializeWow64();
    ExitOnFailure(hr, "Failed to initialize Wow64.");
    initializedWow64 = true;

    hr = WcaDisableWow64FSRedirection();
    ExitOnFailure(hr, "Failed to disable Wow64 redirection.");
    enabled64BitRedirection = true;

    // anything to do?
    if (S_OK != WcaTableExists(L"JsonFile"))
    {
        WcaLog(LOGMSG_STANDARD, "JsonFile table doesn't exist, so there are no .json files to update.");
        ExitFunction();
    }

    // query and loop through all the remove folders exceptions
    hr = WcaOpenExecuteView(vcsJsonFileQuery, &hView);
    ExitOnFailure(hr, "Failed to open view on JsonFile table");

    while (S_OK == (hr = WcaFetchRecord(hView, &hRec)))
    {
        WcaLog(LOGMSG_STANDARD, "Getting JsonFile Id.");
        hr = WcaGetRecordString(hRec, jfqId, &sczId);
        ExitOnFailure(hr, "Failed to get JsonFile identity.");

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile File for Id:%ls", sczId);
        hr = WcaGetRecordFormattedString(hRec, jfqFile, &sczFile);
        ExitOnFailure1(hr, "failed to get File for JsonFile with Id: %s", sczId);

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile ElementPath for Id:%ls", sczId);
        hr = WcaGetRecordString(hRec, jfqElementPath, &sczElementPath);
        ExitOnFailure1(hr, "Failed to get ElementPath for JsonFile with Id: %s", sczId);

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile VerifyPath for Id:%ls", sczId);
        hr = WcaGetRecordString(hRec, jfqVerifyPath, &sczVerifyPath);
        ExitOnFailure1(hr, "Failed to get VerifyPath for JsonFile with Id: %s", sczId);

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile Value for Id:%ls", sczId);
        hr = WcaGetRecordFormattedString(hRec, jfqValue, &sczValue);
        ExitOnFailure1(hr, "Failed to get Value for JsonFile with Id: %s", sczId);

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile Flags for Id:%ls", sczId);
        hr = WcaGetRecordInteger(hRec, jfqFlags, &iFlags);
        ExitOnFailure1(hr, "Failed to get Flags for JsonFile with Id: %s", sczId);

        WcaLog(LOGMSG_STANDARD, "Getting JsonFile Component for Id:%ls", sczId);
        hr = WcaGetRecordString(hRec, jfqComponent, &sczComponent);
        ExitOnFailure(hr, "Failed to get remove folder component.");

        UINT er = ::MsiGetComponentStateW(hInstall, sczComponent, &isInstalled, &isAction);
        ExitOnFailure1(hr = HRESULT_FROM_WIN32(er), "failed to get install state for Component: %ls", sczComponent);
        if (WcaIsInstalling(isInstalled, isAction))
        {
            WcaLog(LOGMSG_STANDARD, "Updating JsonFile for Id:%ls", sczId);
            hr = UpdateJsonFile(sczId, std::filesystem::path{sczFile}, sczElementPath, sczVerifyPath, sczValue, iFlags,
                                sczComponent);
            ExitOnFailure2(hr, "Failed while navigating path: %S for row: %S", sczFile, sczId);
        }
        else if (WcaIsUninstalling(isInstalled, isAction))
        {
            // Don't really worry about this yet as file is deleted on uninstall
        }
    }

    // reaching the end of the list is actually a good thing, not an error
    if (E_NOMOREITEMS == hr)
    {
        hr = S_OK;
    }
    ExitOnFailure(hr, "Failure occured while processing JsonFile table");

LExit:
    if (enabled64BitRedirection)
    {
        WcaRevertWow64FSRedirection();
    }
    if (initializedWow64)
    {
        WcaFinalizeWow64();
    }
    ReleaseStr(sczId)
    ReleaseStr(sczFile)
    ReleaseStr(sczElementPath)
    ReleaseStr(sczVerifyPath)
    ReleaseStr(sczValue)
    ReleaseStr(sczComponent)

    if (hView)
    {
        ::MsiCloseHandle(hView);
        WcaLog(LOGMSG_STANDARD, "Closed PMSIHANDLE hView");
    }

    if (hRec)
    {
        ::MsiCloseHandle(hRec);
        WcaLog(LOGMSG_STANDARD, "Closed PMSIHANDLE hRec");
    }

    DWORD er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
    return WcaFinalize(er);
}


static HRESULT UpdateJsonFile(
    __in_z LPCWSTR wzId,
    __in_z std::filesystem::path wzFile,
    __in_z LPCWSTR wzElementPath,
    __in_z LPCWSTR wzVerifyPath,
    __in_z std::wstring_view wzValue,
    __in int iFlags,
    __in_z std::wstring_view wzComponent
)
{
    HRESULT hr = S_OK;
    ojson j = NULL;
    ::SetLastError(0);


    auto const cFile{ConvertToAnsi(wzFile)};

    std::ifstream is{cFile};
    WcaLog(LOGMSG_STANDARD, "Created ifstream, %s", cFile.c_str());

    json_reader reader(is);

    std::error_code ec;
    reader.read(ec);

    if (ec)
    {
        WcaLog(LOGMSG_STANDARD, "%s on line %i and column %i", ec.message().c_str(), reader.line(), reader.column());
        std::cout << ec.message()
            << " on line " << reader.line()
            << " and column " << reader.column()
            << std::endl;
    }

    is.close();

    std::bitset<32> flags(iFlags);
    WcaLog(LOGMSG_STANDARD, "Using the following flags, %i, %s", iFlags, flags.test(FLAG_SETVALUE) ? "true" : "false");

    _bstr_t bElementPath(wzElementPath);
    char* cElementPath = bElementPath;

    std::string elementPath(cElementPath);

    WcaLog(LOGMSG_STANDARD, "Found ElementPath as %s", elementPath.c_str());
    elementPath = std::regex_replace(elementPath, std::regex(R"(\[(\\\[)\])"), "[");
    elementPath = std::regex_replace(elementPath, std::regex(R"(\[(\\\])\])"), "]");
    WcaLog(LOGMSG_STANDARD, "Updated ElementPath to %s", elementPath.c_str());

    if (flags.test(FLAG_SETVALUE))
    {
        hr = SetJsonPathValue(wzFile, elementPath, wzValue, flags);
    }
    else if (flags.test(FLAG_DELETEVALUE))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
        WcaLog(LOGMSG_STANDARD, "Action deleteValue is not yet supported");
    }
    else if (flags.test(FLAG_JSONPOINTER))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
        WcaLog(LOGMSG_STANDARD, "Type jsonPointer is not yet supported");
    }


    return hr;
}

using namespace jsoncons::jsonpath;

HRESULT SetJsonPathValue(__in_z std::filesystem::path const& wzFile, std::string_view sElementPath,
                      __in_z std::wstring_view wzValue, std::bitset<32> flags)
{
    try
    {
        auto cFile{ConvertToAnsi(wzFile)};

        if (fs::exists(fs::path(wzFile)))
        {
            std::ifstream is{cFile};
            ojson j;

            is >> j;

            if (flags.test(FLAG_JSONBOOL))
            {
                auto cValue{wzValue == std::wstring_view{ L"true" } || wzValue == std::wstring_view{ L"1" } };

                WcaLog(LOGMSG_STANDARD, "About to replace boolean value: |%s| {\"%S\" as  %s}", sElementPath.data(),
                       wzValue.data(), (cValue ? "true" : "false"));

                json_replace(j, sElementPath, cValue);
                WcaLog(LOGMSG_STANDARD, "Updating the json %s with value %s.", sElementPath.data(),
                       (cValue ? "true" : "false"));
            }
            else if (flags.test(FLAG_JSONNUMBER))
            {
                auto cValue{std::stoi(ConvertToAnsi(wzValue))};

                WcaLog(LOGMSG_STANDARD, "About to replace number value: |%s| {\"%S\" as %ld}", sElementPath.data(),
                       wzValue.data(), cValue);

                json_replace(j, sElementPath, cValue);
                WcaLog(LOGMSG_STANDARD, "Updating the json %s with value %ld.", sElementPath.data(), cValue);
            }
            else if (flags.test(FLAG_JSONOBJECT))
            {
                throw new std::runtime_error("Object type is not yet supported.");
            }
            else if (flags.test(FLAG_JSONNULL))
            {
                WcaLog(LOGMSG_STANDARD, "About to replace null value: |%s|", sElementPath.data());

                json_replace(j, sElementPath, nullptr);
                WcaLog(LOGMSG_STANDARD, "Updating the json %s with null value.", sElementPath.data());
            }
            else
            {
                auto cValue{ConvertToAnsi(wzValue)};
                WcaLog(LOGMSG_STANDARD, "About to replace text value: |%s| {\"%s\"}", sElementPath.data(), cValue.c_str());

                json_replace(j, sElementPath, cValue);
                WcaLog(LOGMSG_STANDARD, "Updating the json %s with value %s.", sElementPath.data(), cValue.c_str());
            }

            WcaLog(LOGMSG_STANDARD, "creating output stream %s", cFile.c_str());
            std::ofstream os(cFile,
                             std::ios_base::out | std::ios_base::trunc);
            WcaLog(LOGMSG_STANDARD, "created output stream %s", cFile.c_str());

            pretty_print(j).dump(os);
            WcaLog(LOGMSG_STANDARD, "dumped output stream");

            os.close();
            WcaLog(LOGMSG_STANDARD, "closed output stream");
        }
        else
        {
            WcaLog(LOGMSG_STANDARD, "Unable to locate file: %s", cFile.c_str());
        }
    }
    catch (std::exception& e)
    {
        WcaLog(LOGMSG_STANDARD, "encountered error %s", e.what());
        return HRESULT_FROM_WIN32(ERROR_INSTALL_FAILED);
    }
    return S_OK;
}
