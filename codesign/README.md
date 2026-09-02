# Code-signing certificate

`DcvLdiExtension.cer` is the **same** certificate used by the .NET
`dcv-ldi-extension` project (`CN=Amazon Web Services, Inc.`, thumbprint
`E6B2B8201DB1843DA499DE8252B60DA4B806E2F7`, valid until 2036-09-02).

Reusing it here means a machine that already imported it for the .NET build
trusts binaries from this C++ build too, with no extra import step.

## Import on a test machine (run as Administrator)

```powershell
Import-Certificate -FilePath codesign\DcvLdiExtension.cer -CertStoreLocation Cert:\LocalMachine\TrustedPublisher
Import-Certificate -FilePath codesign\DcvLdiExtension.cer -CertStoreLocation Cert:\LocalMachine\Root
```

## Private key (PFX)

Not in this repo. This workflow expects the **same** two GitHub Actions
secrets used by the .NET project's repo:

| Secret | Contents |
|---|---|
| `CODE_SIGNING_PFX_BASE64` | Base64 of the `.pfx` (cert + private key) |
| `CODE_SIGNING_PFX_PASSWORD` | Password protecting the `.pfx` |

If this project lives in a different GitHub repository than the .NET one,
these two secrets need to be added here as well (same values).
