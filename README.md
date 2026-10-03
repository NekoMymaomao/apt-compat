**English** | [中文](README.zh.md)

# apt-compat

A static binary that maps the **apt command line** onto the host's native package manager.

It lets you keep using `apt install / apt search / apt update ...` on non-Debian distributions.

- **Pure mapping layer** — mappable commands and arguments are translated into the backend's form; anything unmappable is **passed through unchanged**.
- **No privilege escalation** — it never adds `sudo`; when root is needed, run `sudo apt ...` yourself.
- **Single static binary** — `ldd` reports `not a dynamic executable`; no runtime dependencies.

---

## Supported Systems & Package Managers

Detection follows the table order; the first one found wins.

| System | Package manager | Probe |
|---|---|---|
| Fedora, RHEL 8+, CentOS Stream, Rocky, AlmaLinux, Amazon Linux, Oracle Linux | dnf | `dnf` |
| older RHEL/CentOS 7 | yum | `yum` |
| Arch, Manjaro, EndeavourOS, Garuda, MSYS2 | pacman | `pacman` |
| openSUSE Leap / Tumbleweed, SLES | zypper | `zypper` |
| Gentoo, Funtoo | emerge (Portage) | `emerge` |
| Alpine, postmarketOS | apk | `apk` |
| Void Linux | xbps | `xbps-install` |
| OpenWrt | opkg | `opkg` |
| Solus | eopkg | `eopkg` |
| Mageia, OpenMandriva | urpm (`urpmi`/`urpme`/`urpmq`) | `urpmi` |
| Slackware | slackpkg | `slackpkg` |
| NixOS | nix-env | `nix-env` |
| CRUX | prt-get | `prt-get` |
| Guix System | guix | `guix` |
| FreeBSD, GhostBSD | pkg | `pkg` |
| Haiku | pkgman | `pkgman` |
| NuTyX | cards | `cards` |

> Note: if several supported backends exist (e.g. both dnf and yum), the one earlier in the table is chosen.

---

## Command Mapping

### Installation, Removal & Query

| Backend | install | remove | purge | search | show |
|---|---|---|---|---|---|
| dnf | `dnf install` | `dnf remove` | `dnf remove` | `dnf search` | `dnf info` |
| yum | `yum install` | `yum remove` | `yum remove` | `yum search` | `yum info` |
| pacman | `pacman -S` | `pacman -R` | `pacman -Rn` | `pacman -Ss` | `pacman -Si` |
| zypper | `zypper install` | `zypper remove` | `zypper remove` | `zypper search` | `zypper info` |
| emerge | `emerge` | `emerge --unmerge` | `emerge --unmerge` | `emerge --search` | `emerge -pv` |
| apk | `apk add` | `apk del` | `apk del` | `apk search` | `apk info` |
| xbps | `xbps-install` | `xbps-remove` | `xbps-remove` | `xbps-query -Rs` | `xbps-query -RS` |
| opkg | `opkg install` | `opkg remove` | `opkg remove` | `opkg find` | `opkg info` |
| eopkg | `eopkg install` | `eopkg remove` | `eopkg remove` | `eopkg search` | `eopkg info` |
| urpm | `urpmi` | `urpme` | `urpme` | `urpmq -y` | `urpmq -i` |
| slackpkg | `slackpkg install` | `slackpkg remove` | `slackpkg remove` | `slackpkg search` | `slackpkg info` |
| nix-env | `nix-env -i` | `nix-env -e` | `nix-env -e` | `nix-env -qaP` | `nix-env -qa --description` |
| prt-get | `prt-get install` | `prt-get remove` | `prt-get remove` | `prt-get search` | `prt-get info` |
| guix | `guix install` | `guix remove` | `guix remove` | `guix search` | `guix show` |
| pkg | `pkg install` | `pkg remove` | `pkg remove` | `pkg search` | `pkg info` |
| pkgman | `pkgman install` | `pkgman uninstall` | `pkgman uninstall` | `pkgman search` | — |
| cards | `cards install` | `cards remove` | `cards remove` | `cards search` | `cards info` |

### Update Index, Upgrade & List

| Backend | update | upgrade | full-upgrade | list --installed |
|---|---|---|---|---|
| dnf | `dnf -q makecache` | `dnf upgrade` | `dnf distro-sync` | `dnf -q list installed` |
| yum | `yum -q makecache` | `yum update` | `yum update` | `yum -q list installed` |
| pacman | `pacman -Sy` | `pacman -Su` | `pacman -Syu` | `pacman -Q` |
| zypper | `zypper --quiet refresh` | `zypper update` | `zypper dup` | `zypper --quiet pa -i` |
| emerge | `emerge --sync` | `emerge -u @world` | `emerge -uDN @world` | scan `/var/db/pkg` |
| apk | `apk update` | `apk upgrade` | `apk upgrade` | `apk list --installed` |
| xbps | `xbps-install -S` | `xbps-install -u` | `xbps-install -Su` | `xbps-query -l` |
| opkg | `opkg update` | `opkg upgrade` | `opkg upgrade` | `opkg list-installed` |
| eopkg | `eopkg update-repo` | `eopkg upgrade` | `eopkg upgrade` | `eopkg list-installed` |
| urpm | `urpmi.update -a` | `urpmi --auto-update` | `urpmi --auto-update` | `rpm -q --all` |
| slackpkg | `slackpkg update` | `slackpkg upgrade-all` | `slackpkg upgrade-all` | scan `/var/log/packages` |
| nix-env | `nix-channel --update` | `nix-env -u` | `nix-env -u` | `nix-env -q --installed` |
| prt-get | `ports -u` | `prt-get sysup` | `prt-get sysup` | `prt-get listinst` |
| guix | `guix refresh` | `guix upgrade` | `guix upgrade` | `guix package -I` |
| pkg | `pkg update` | `pkg upgrade` | `pkg upgrade` | `pkg info -a` |
| pkgman | `pkgman refresh` | `pkgman update` | `pkgman update` | `pkgman search -i -a` |
| cards | `cards sync` | `cards upgrade` | `cards upgrade` | `cards list` |

> The full per-operation mapping (including reinstall / download / clean / list --upgradable) lives in the `g_backends[]` table in `apt-backends.c`.

---

## Option Mapping

| apt option | Meaning | How it maps |
|---|---|---|
| `-y`, `--yes`, `--assume-yes` | answer yes to all | append the backend's non-interactive flag (e.g. `-y`, `--noconfirm`, `--non-interactive`) |
| `-s`, `--simulate`, `--dry-run`, `--just-print` | simulate | append the backend's simulate flag (e.g. `--assumeno`, `--print`, `--dry-run`) |
| `-d`, `--download-only` | download only | use the backend download subcommand (e.g. `dnf download`, `pacman -Sw`, `xbps-install -D`) |
| `--purge` / `purge` | remove config too | use the purge template (e.g. `pacman -Rn`), else fall back to remove |
| `--reinstall` / `reinstall` | reinstall | use the reinstall template, else fall back to install |
| `--only-upgrade` | upgrade only if installed | use the backend's per-package upgrade (e.g. `dnf upgrade <pkg>`) |
| `--installed` | list installed only | `list` uses list_installed |
| `--upgradable` | list upgradable only | `list` uses list_upgradable |
| `-o KEY=VALUE` | config option | consumed (next arg skipped) |
| `-q`, `--quiet`, `--no-install-recommends` | — | apt-specific, ignored |
| any other unknown option | — | **passed through** to the backend |
| args after `--` | — | **passed through** to the backend, appended at the end |

Example:

```sh
apt install vim -- --proxy=localhost:1234   # -> apk add vim --proxy=localhost:1234
apt install -y vim=9.0.1                    # -> apk add vim=9.0.1  (see Version pinning)
```

---

## Cloud Build (GitHub Actions)

The repo ships a workflow `.github/workflows/build.yml`; after forking, anyone can cloud-compile the static binaries on GitHub in one click, with no local toolchain.

- Triggers: push to `master`/`main`, a pull request, or a manual `Run workflow` from the Actions page.
- Artifacts: for each architecture (`x86_64`, `aarch64`) it produces `apt-<arch>-musl` and `apt-<arch>-glibc`.
- Releases: at the end of the workflow the binaries are published to Releases. Pushing to the main branch updates a rolling `latest` pre-release (always carrying the newest binaries); pushing a tag like `v1.0.0` creates a proper Release.
- Verify: the workflow runs `file` + `ldd` to confirm static linking and prints `--version`.

---

## Notes

1. **No privilege escalation.**
   The program never calls `sudo`. After installing it as `/usr/bin/apt`, prefix privileged ops with `sudo apt ...`; running as a normal user without sudo lets the backend report the permission error, exactly as running the backend directly would.

2. **Version pinning (`pkg=version`).**
   dnf/yum/xbps use `name-version`; zypper/apk use `name=version`; emerge uses `=name-version`. The remaining backends (pacman/slackpkg/urpm/nix-env/prt-get/guix/pkg/pkgman/cards/opkg/eopkg) have no equivalent form: the version is stripped, a `W:` warning is printed, and the latest version is installed.

3. **Unsupported operations.**
   Operations marked unsupported (empty `{0}` templates) never fabricate a command; they print a `W:`/`E:` warning or fall back to an equivalent.

4. **Special reproductions.**
   - pacman has no single autoremove command, so it is reproduced as list-orphans-then-remove (`pacman -Qdtq` -> `pacman -R`).
   - emerge's installed list scans `/var/db/pkg`; slackpkg's scans `/var/log/packages`.
   - apk has no autoremove concept; it just warns and does nothing.

5. **stdout, stderr and exit codes.**
   The tool only dispatches and executes the backend command; its output and exit code are forwarded as-is.

6. **Build & Install.**

   ```sh
   make                    # recommended
   # or equivalently:
   gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   # cleaner static link with musl:
   musl-gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   ```

   Install onto a non-apt system:

   ```sh
   [ -e /usr/bin/dpkg ] && { echo "Debian-family detected, abort."; exit 1; }
   sudo install -m 0755 apt /usr/bin/apt
   sudo ln -sf apt /usr/bin/apt-get
   sudo ln -sf apt /usr/bin/apt-cache
   ```

7. **References.**
   Mapping rules reference: `icy/pacapt`, `Itai-Nelken/aptpac`, `rami3l/pacaptr`, `sigoden/upt` (the authoritative per-operation table is `reference/upt/src/vendor.rs`).
