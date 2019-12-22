
#pragma once

#include <cstdint>
#include "os.h"
#include "resource.h"
#include "cryptuiapi.h"

/**
 * Loads an embedded X.509 certificate from a resource in the executable into a Win32 Crypto API certificate
 * context for further use.
 */
class CertificateContext {
public:
    CertificateContext();

    ~CertificateContext();

    void LoadFromResource();

    void ShowDialog(HWND parentWindow);

    PCCERT_CONTEXT GetHandle() const {
        return ctx;
    }

    void Free() noexcept;

    bool IsInstalled();

    CertificateContext(CertificateContext &) = delete;

    CertificateContext &operator=(CertificateContext &) = delete;

private:
    PCCERT_CONTEXT ctx = nullptr;

    static void GetRawResourceData(LPCWSTR name, LPCWSTR type, const uint8_t **data, uint32_t *size);

};

class CertificateStore {
public:

    CertificateStore(bool readOnly) {

        if (readOnly) {
            store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
                                  CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG, L"ROOT");
        } else {
            store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"ROOT");
        }

        if (!store) {
            throw std::system_error(last_error_code(), "CertOpenStore failed");
        }
    }

    ~CertificateStore() {
        if (store) {
            CertCloseStore(store, CERT_CLOSE_STORE_FORCE_FLAG);
            store = nullptr;
        }
    }

    bool Contains(const CertificateContext &cert) const;

    void Install(CertificateContext &cert) const;

private:
    HCERTSTORE store = nullptr;

};

inline CertificateContext::~CertificateContext() {
    Free();
}

inline void CertificateContext::GetRawResourceData(LPCWSTR name, LPCWSTR type, const uint8_t **data, uint32_t *size) {
    auto resInfo = FindResource(nullptr, name, type);
    if (!resInfo) {
        throw std::system_error(
                last_error_code(),
                "FindResource failed while trying to load the embedded certificate."
        );
    }

    auto resHandle = LoadResource(nullptr, resInfo);
    if (!resHandle) {
        throw std::system_error(
                last_error_code(),
                "LoadResource failed while trying to load the embedded certificate."
        );
    }

    *size = SizeofResource(nullptr, resInfo);
    *data = static_cast<const uint8_t *>(LockResource(resHandle));
}

inline void CertificateContext::LoadFromResource() {
    Free();

    const uint8_t *certData;
    unsigned int certSize;
    GetRawResourceData(MAKEINTRESOURCE(RES_CERT), RT_RCDATA, &certData, &certSize);

    ctx = CertCreateCertificateContext(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            certData,
            certSize
    );

    if (!ctx) {
        throw std::system_error(
                last_error_code(),
                "CertCreateCertificateContext failed while trying to load the embedded certificate."
        );
    }
}

inline void CertificateContext::Free() noexcept {
    if (ctx) {
        CertFreeCertificateContext(ctx);
        ctx = nullptr;
    }
}

void CertificateContext::ShowDialog(HWND parentWindow) {

    if (!ctx) {
        throw std::exception("Cannot show certificate before loading it.");
    }

    CryptUIDlgViewContext(
            CERT_STORE_CERTIFICATE_CONTEXT,
            ctx,
            parentWindow,
            L"OpenTemple Nightly Build Certificate",
            0,
            nullptr
    );

}

CertificateContext::CertificateContext() = default;

inline bool CertificateStore::Contains(const CertificateContext &cert) const {

    auto foundCert = CertFindCertificateInStore(
            store,
            X509_ASN_ENCODING,
            0,
            CERT_FIND_EXISTING,
            cert.GetHandle(),
            nullptr
    );

    if (foundCert) {
        CertFreeCertificateContext(foundCert);
        return true;
    }

    return false;
}

void CertificateStore::Install(CertificateContext &cert) const {
    auto result = CertAddCertificateContextToStore(
            store,
            cert.GetHandle(),
            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
            nullptr
    );
    if (!result) {
        throw std::system_error(last_error_code(), "CertAddCertificateContextToStore failed");
    }
}
