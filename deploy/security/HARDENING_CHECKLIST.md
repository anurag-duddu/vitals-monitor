# Security Hardening Checklist — Vitals Monitor

**CDSCO Class B Medical Device | STM32MP157F-DK2**
**IEC 62443-4-2 / IEC 62304 / IEC 80001-1 Compliance**

This checklist must be completed and signed off before every production deployment.
Each item maps to a specific regulatory requirement and security control.

---

## 1. Secure Boot Chain

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 1.1 | STM32MP1 OTP secure boot fuses are programmed (BSEC) | IEC 62443-4-2 CR 3.4 | [ ] |
| 1.2 | TF-A (BL2) is signed with production RSA-2048/ECDSA key | IEC 62443-4-2 CR 3.4 | [ ] |
| 1.3 | U-Boot binary is signed and verified by TF-A | IEC 62443-4-2 CR 3.4 | [ ] |
| 1.4 | FIT image (kernel + DTB) is signed with production key | IEC 62443-4-2 CR 3.4 | [ ] |
| 1.5 | Debug boot paths are disabled (no unsigned fallback) | IEC 62443-4-2 CR 1.5 | [ ] |
| 1.6 | Secure boot keys are stored in HSM (not on build server) | IEC 62443-4-2 CR 1.7 | [ ] |
| 1.7 | Key revocation mechanism is documented and tested | IEC 62443-4-2 CR 1.9 | [ ] |

## 2. dm-verity (Root Filesystem Integrity)

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 2.1 | Root filesystem is mounted read-only | IEC 62443-4-2 CR 3.4 | [ ] |
| 2.2 | dm-verity hash tree is generated (`setup-dm-verity.sh`) | IEC 62443-4-2 CR 3.4 | [ ] |
| 2.3 | Root hash is embedded in the signed FIT image | IEC 62443-4-2 CR 3.4 | [ ] |
| 2.4 | Kernel cmdline includes `dm-mod.create` with correct root hash | IEC 62443-4-2 CR 3.4 | [ ] |
| 2.5 | dm-verity restart-on-corruption mode is enabled | IEC 62443-4-2 CR 3.4 | [ ] |
| 2.6 | Verification tested: modified rootfs block triggers I/O error | IEC 62443-4-2 CR 3.4 | [ ] |

## 3. LUKS (Data-at-Rest Encryption)

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 3.1 | Data partition is LUKS2-encrypted (`setup-luks.sh`) | IEC 62443-4-2 CR 4.1 | [ ] |
| 3.2 | Encryption key derived from hardware unique ID (OTP fuses) | IEC 62443-4-2 CR 1.7 | [ ] |
| 3.3 | Cipher is AES-256-XTS (FIPS 140-2 approved algorithm) | IEC 62443-4-2 CR 4.1 | [ ] |
| 3.4 | PBKDF is Argon2id with parameters tuned for target CPU | IEC 62443-4-2 CR 4.1 | [ ] |
| 3.5 | LUKS container auto-unlocks at boot via hardware ID | IEC 62443-4-2 CR 4.1 | [ ] |
| 3.6 | No key files stored on the filesystem | IEC 62443-4-2 CR 1.7 | [ ] |
| 3.7 | Data partition cannot be decrypted on another device | IEC 62443-4-2 CR 4.1 | [ ] |

## 4. AppArmor (Mandatory Access Control)

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 4.1 | AppArmor is enabled in kernel config (`CONFIG_SECURITY_APPARMOR=y`) | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.2 | AppArmor is set to enforce mode (not complain mode) | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.3 | `vitals-ui` profile is loaded and enforcing | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.4 | `sensor-service` profile is loaded and enforcing | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.5 | `alarm-service` profile is loaded and enforcing | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.6 | `network-service` profile is loaded and enforcing | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.7 | `watchdog-monitor` profile is loaded and enforcing | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.8 | All deny rules tested: violations generate audit log entries | IEC 62443-4-2 CR 2.1 | [ ] |
| 4.9 | No unconfined processes running in production | IEC 62443-4-2 CR 2.1 | [ ] |

## 5. Debug Interfaces Disabled

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 5.1 | JTAG/SWD debug port is disabled via OTP fuses | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.2 | Serial console (UART4) is disabled or password-protected | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.3 | SSH server is not installed (or disabled) | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.4 | GDB/strace/ltrace are not installed on target | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.5 | `/proc/kcore` and `/proc/kmem` are not accessible | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.6 | `CONFIG_DEBUG_FS` is disabled in kernel config | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.7 | `kernel.kptr_restrict=2` is set (hide kernel pointers) | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.8 | `kernel.dmesg_restrict=1` is set (restrict dmesg access) | IEC 62443-4-2 CR 1.5 | [ ] |
| 5.9 | U-Boot autoboot delay is 0 (no interactive prompt) | IEC 62443-4-2 CR 1.5 | [ ] |

## 6. Network Hardening

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 6.1 | Only `network-service` has network access (AppArmor enforced) | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.2 | No listening TCP/UDP ports (verified with `ss -tlnp`) | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.3 | Outbound connections use TLS 1.2+ only | IEC 62443-4-2 CR 4.1 | [ ] |
| 6.4 | TLS certificate pinning is configured for known servers | IEC 62443-4-2 CR 4.1 | [ ] |
| 6.5 | iptables/nftables default policy is DROP (ingress + egress) | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.6 | Only required outbound ports are allowed (443, 53) | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.7 | ICMP is rate-limited or disabled | IEC 62443-4-2 CR 7.6 | [ ] |
| 6.8 | IPv6 is disabled if not used | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.9 | Wi-Fi firmware is not included unless required | IEC 62443-4-2 CR 7.1 | [ ] |
| 6.10 | Bluetooth is disabled at kernel level | IEC 62443-4-2 CR 7.1 | [ ] |

## 7. Kiosk Mode (Single-Application Lockdown)

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 7.1 | `vitals-ui` is the only graphical application | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.2 | No window manager or desktop environment installed | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.3 | Virtual terminals (VT switching) are disabled | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.4 | Ctrl+Alt+Del is remapped or ignored | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.5 | Magic SysRq keys are disabled (`kernel.sysrq=0`) | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.6 | USB mass storage is disabled (`modprobe.blacklist=usb-storage`) | IEC 62443-4-2 CR 2.1 | [ ] |
| 7.7 | Only authorized USB HID devices are allowed (if any) | IEC 62443-4-2 CR 2.1 | [ ] |

## 8. Audit Trail

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 8.1 | All alarm events are logged with timestamps (IEC 60601-1-8) | IEC 62304 5.7.5 | [ ] |
| 8.2 | Configuration changes are logged with before/after values | IEC 62443-4-2 CR 6.1 | [ ] |
| 8.3 | AppArmor violations are logged via auditd | IEC 62443-4-2 CR 6.1 | [ ] |
| 8.4 | Log storage is on the encrypted data partition | IEC 62443-4-2 CR 4.1 | [ ] |
| 8.5 | Log rotation is configured (prevents disk full) | IEC 62443-4-2 CR 6.1 | [ ] |
| 8.6 | Logs cannot be modified by non-root processes | IEC 62443-4-2 CR 6.1 | [ ] |
| 8.7 | Journald persistent storage is enabled | IEC 62443-4-2 CR 6.1 | [ ] |
| 8.8 | Minimum 72 hours of audit trail retained | IEC 62304 5.7.5 | [ ] |

## 9. OTA Update Verification

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 9.1 | OTA updates are signed with the production signing key | IEC 62443-4-2 CR 3.4 | [ ] |
| 9.2 | Update signature is verified before installation | IEC 62443-4-2 CR 3.4 | [ ] |
| 9.3 | Rollback mechanism is available (A/B partition scheme) | IEC 62443-4-2 CR 3.14 | [ ] |
| 9.4 | Update transport uses TLS 1.2+ with cert pinning | IEC 62443-4-2 CR 4.1 | [ ] |
| 9.5 | Failed update does not brick the device (watchdog recovery) | IEC 62443-4-2 CR 3.14 | [ ] |
| 9.6 | Update version is checked (no downgrade attacks) | IEC 62443-4-2 CR 3.4 | [ ] |
| 9.7 | Post-update verification confirms successful boot | IEC 62443-4-2 CR 3.4 | [ ] |

## 10. Resource Protection

| # | Check | Standard | Status |
|---|-------|----------|--------|
| 10.1 | systemd `MemoryMax` is set for all services | IEC 62443-4-2 CR 7.7 | [ ] |
| 10.2 | systemd `LimitNOFILE` is set for all services | IEC 62443-4-2 CR 7.7 | [ ] |
| 10.3 | Watchdog timer resets device if critical service hangs | IEC 62443-4-2 CR 7.7 | [ ] |
| 10.4 | `NoNewPrivileges=yes` is set for all non-root services | IEC 62443-4-2 CR 2.1 | [ ] |
| 10.5 | `ProtectSystem=strict` is set for all services | IEC 62443-4-2 CR 2.1 | [ ] |
| 10.6 | Stack canaries enabled (`-fstack-protector-strong`) | IEC 62443-4-2 CR 3.4 | [ ] |
| 10.7 | ASLR is enabled (`kernel.randomize_va_space=2`) | IEC 62443-4-2 CR 3.4 | [ ] |
| 10.8 | PIE is enabled for all binaries (`-fPIE -pie`) | IEC 62443-4-2 CR 3.4 | [ ] |
| 10.9 | FORTIFY_SOURCE is enabled (`-D_FORTIFY_SOURCE=2`) | IEC 62443-4-2 CR 3.4 | [ ] |
| 10.10 | RELRO is enabled (`-Wl,-z,relro,-z,now`) | IEC 62443-4-2 CR 3.4 | [ ] |

---

## Sign-Off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Software Engineer | | | |
| Security Reviewer | | | |
| Quality Assurance | | | |
| Regulatory Affairs | | | |

---

*This checklist is a living document. Update it whenever the security architecture changes.*
*Version: 1.0 | Last Updated: 2026-02-08*
