# EU Cyber Resilience Act: what this starter covers

> **Not legal advice.** This is an engineering checklist written against
> Regulation (EU) 2024/2847. Check the official text (EUR-Lex) and, for your
> product category, the harmonised standards and any implementing acts before
> relying on it.

## Dates

- **11 September 2026:** reporting obligations apply (actively exploited
  vulnerabilities and severe incidents, via the ENISA single reporting platform).
- **11 December 2027:** all other obligations apply: essential cybersecurity
  requirements, conformity assessment, CE marking, technical documentation.
- Products placed on the market before 11 December 2027 are covered by the
  full requirements only if substantially modified afterwards. Reporting
  obligations apply to them anyway.
- Out of scope: products covered by their own regimes (e.g. medical devices,
  motor vehicles, aviation, marine equipment).

## Requirement → what the starter gives you → what you still own

| CRA requirement (Annex I) | Provided here | Still yours |
|---|---|---|
| No known exploitable vulnerabilities at release | SPDX SBOM per image (`build/sbom/`), pinned dependencies (`west.yml`) | Run a vulnerability scan against the SBOM before each release and act on it |
| Secure by default configuration | Release builds signed with your key; debug features listed below | Disable the UART shell / enable APPROTECT on production units (see "Production hardening") |
| Protection against unauthorised access / integrity | MCUboot verifies ECDSA P-256 signatures on every boot. Twin tests prove that tampered images and images signed with another key are rejected | Key management: generate offline, store in CI secret + offline backup, rotate plan |
| Security updates, installable securely, automatic rollback | Signed images, A/B slots, self-test then `boot_write_img_confirmed()`; unconfirmed images revert on reset | The update transport (BLE SMP, cloud). MCUmgr is a config change away |
| Anti-rollback (where appropriate) | Documented option: `CONFIG_MCUBOOT_DOWNGRADE_PREVENTION` (needs overwrite-only upgrades) | Decide based on your risk assessment |
| Confidentiality / data minimisation | Only sensor data, no personal data collected by default | BLE pairing/bonding policy if your data is sensitive |
| Limit attack surface | Only the modules in `west.yml` allowlist are fetched; BLE GATT is read-only | Review every new block/module |
| Logging/monitoring of security events | Zephyr logging, boot log shows image version and verification | Decide what is kept on the device and reported |
| **Vulnerability handling (Annex I Part II)** | | |
| Identify and document components (SBOM) | `scripts/build.sh` writes SPDX 2.3 for application and bootloader, release workflow attaches it | Keep SBOMs for every released version |
| Coordinated disclosure policy + contact | `SECURITY.md` template | Fill it in and publish it |
| Address vulnerabilities without delay, free security updates | Release workflow: tag → build → twin tests on the exact release images → signed artifacts + SBOM + checksums | Support period (normally at least 5 years) and update policy |
| Report actively exploited vulnerabilities | n/a (process) | 24 h early warning, 72 h notification, final report: name an owner |

## Production hardening (do before shipping)

1. Generate the production signing key offline:
   `imgtool keygen -k key.pem -t ecdsa-p256`. Store it as the
   `MCUBOOT_SIGNING_KEY` repository secret (environment `release`) and keep an
   offline backup. **Never** ship with MCUboot's default development key
   (that's what local builds use; anyone can sign images with it).
2. Disable or lock the debug port: enable APPROTECT on the nRF52840 during
   production programming.
3. Decide on the UART shell: keep it for factory test, disable for shipping
   (`CONFIG_SHELL=n` in a `prj_release.conf` overlay), or protect it.
4. Turn on anti-rollback if your risk assessment says so.
5. Fill in `SECURITY.md` and the support period.

## Evidence produced automatically

| Evidence | Where |
|---|---|
| SBOM (SPDX 2.3), app + bootloader | CI artifact / release asset `sbom-spdx.tar.gz` |
| Secure boot rejects tampered and foreign-key images | `tests/twin/boot.robot`, results in CI artifact `twin-results/` |
| Image integrity | `SHA256SUMS` in each release |
| Hardware/firmware consistency | `west hwsync check` + `verify-build` logs in CI |
