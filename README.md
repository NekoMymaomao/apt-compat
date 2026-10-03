# apt-compat

一个把 **apt 命令行**映射到本机包管理器的静态二进制。
A static binary that maps the **apt command line** onto the native package manager of the host.

它让你在非 Debian 系发行版上照常敲 `apt install / apt search / apt update ...`。
It lets you keep typing `apt install / apt search / apt update ...` on non-Debian distributions.

- **纯映射层 / Pure mapping layer**：能对应上的命令与参数映射成后端写法，对应不上的**原样透传**。
  Mappable commands/arguments are translated; anything unmappable is **passed through unchanged**.
- **不代提权 / No privilege escalation**：绝不自动加 `sudo`；需要 root 时由你自己 `sudo apt ...`。
  It never prepends `sudo`; when root is needed, you run `sudo apt ...` yourself.
- **单文件静态二进制 / Single static binary**：`ldd` 显示 `not a dynamic executable`，无运行时依赖。
  `ldd` reports `not a dynamic executable`; no runtime dependencies.

---

## 支持的系统与包管理器 / Supported Systems & Package Managers

按表中顺序探测；命中第一个存在的即选中。
Detection follows the table order; the first one found wins.

| 系统 / System | 包管理器 / Package manager | 探测命令 / Probe |
|---|---|---|
| Fedora, RHEL 8+, CentOS Stream, Rocky, AlmaLinux, Amazon Linux, Oracle Linux | dnf | `dnf` |
| 旧版 RHEL/CentOS 7 / older RHEL/CentOS 7 | yum | `yum` |
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

> 注意 / Note：一个系统若同时装了多个受支持的后端（例如装了 dnf 和 yum），按上表顺序优先取靠前者。
> If several supported backends exist (e.g. both dnf and yum), the one earlier in the table is chosen.

---

## 命令映射 / Command Mapping

### 安装 / 卸载 / 查询 / Installation, Removal & Query

| 后端 / Backend | install | remove | purge | search | show |
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

### 更新索引 / 升级 / 列表 / Update Index, Upgrade & List

| 后端 / Backend | update | upgrade | full-upgrade | list --installed |
|---|---|---|---|---|
| dnf | `dnf -q makecache` | `dnf upgrade` | `dnf distro-sync` | `dnf -q list installed` |
| yum | `yum -q makecache` | `yum update` | `yum update` | `yum -q list installed` |
| pacman | `pacman -Sy` | `pacman -Su` | `pacman -Syu` | `pacman -Q` |
| zypper | `zypper --quiet refresh` | `zypper update` | `zypper dup` | `zypper --quiet pa -i` |
| emerge | `emerge --sync` | `emerge -u @world` | `emerge -uDN @world` | 扫描 `/var/db/pkg` |
| apk | `apk update` | `apk upgrade` | `apk upgrade` | `apk list --installed` |
| xbps | `xbps-install -S` | `xbps-install -u` | `xbps-install -Su` | `xbps-query -l` |
| opkg | `opkg update` | `opkg upgrade` | `opkg upgrade` | `opkg list-installed` |
| eopkg | `eopkg update-repo` | `eopkg upgrade` | `eopkg upgrade` | `eopkg list-installed` |
| urpm | `urpmi.update -a` | `urpmi --auto-update` | `urpmi --auto-update` | `rpm -q --all` |
| slackpkg | `slackpkg update` | `slackpkg upgrade-all` | `slackpkg upgrade-all` | 扫描 `/var/log/packages` |
| nix-env | `nix-channel --update` | `nix-env -u` | `nix-env -u` | `nix-env -q --installed` |
| prt-get | `ports -u` | `prt-get sysup` | `prt-get sysup` | `prt-get listinst` |
| guix | `guix refresh` | `guix upgrade` | `guix upgrade` | `guix package -I` |
| pkg | `pkg update` | `pkg upgrade` | `pkg upgrade` | `pkg info -a` |
| pkgman | `pkgman refresh` | `pkgman update` | `pkgman update` | `pkgman search -i -a` |
| cards | `cards sync` | `cards upgrade` | `cards upgrade` | `cards list` |

> 完整逐项映射（含 reinstall / download / clean / list --upgradable 等）见 `apt-backends.c` 中的映射表。
> The full per-operation mapping (including reinstall / download / clean / list --upgradable) lives in the `g_backends[]` table in `apt-backends.c`.

---

## 选项映射 / Option Mapping

| apt 选项 / apt option | 含义 / Meaning | 映射方式 / How it maps |
|---|---|---|
| `-y`, `--yes`, `--assume-yes` | 全部回答 yes / answer yes to all | 追加后端的非交互标志（如 `-y`、`--noconfirm`、`--non-interactive`）/ append the backend's non-interactive flag |
| `-s`, `--simulate`, `--dry-run`, `--just-print` | 模拟 / simulate | 追加后端的模拟标志（如 `--assumeno`、`--print`、`--dry-run`）/ append the backend's simulate flag |
| `-d`, `--download-only` | 只下载 / download only | 走后端 download 子命令（如 `dnf download`、`pacman -Sw`、`xbps-install -D`）/ use the backend download subcommand |
| `--purge` / `purge` | 连配置一起删 / remove config too | 用后端 purge 模板（如 `pacman -Rn`），无则退回 remove / use the purge template, else fall back to remove |
| `--reinstall` / `reinstall` | 重装 / reinstall | 用后端 reinstall 模板，无则退回 install / use the reinstall template, else fall back to install |
| `--only-upgrade` | 只升级已装的 / upgrade only if installed | 用后端"升级指定包"模板（如 `dnf upgrade <pkg>`）/ use the backend's per-package upgrade |
| `--installed` | 只列已装 / list installed only | `list` 走 list_installed / `list` uses list_installed |
| `--upgradable` | 只列可升级 / list upgradable only | `list` 走 list_upgradable / `list` uses list_upgradable |
| `-o KEY=VALUE` | 配置项 / config option | 消费该参数（跳过）/ consumed (next arg skipped) |
| `-q`, `--quiet`, `--no-install-recommends` | — | apt 专属，忽略 / apt-specific, ignored |
| 其它未知选项 / any other unknown option | — | **原样透传**给后端 / **passed through** to the backend |
| `--` 之后的参数 / args after `--` | — | **原样透传**给后端（追加到命令末尾）/ **passed through**, appended at the end |

示例 / Example：

```sh
apt install vim -- --proxy=localhost:1234   # → apk add vim --proxy=localhost:1234
apt install -y vim=9.0.1                    # → apk add vim=9.0.1  (见下方版本锁定)
```

---

## 云编译 / Cloud Build (GitHub Actions)

仓库自带工作流 `.github/workflows/build.yml`，任何人 Fork 后都能在 GitHub 上一键云端编译静态二进制，无需本地工具链。
The repo ships a workflow `.github/workflows/build.yml`; after forking, anyone can cloud-compile the static binaries on GitHub in one click, with no local toolchain.

- 触发 / Triggers：推送到 `master`/`main`、Pull Request，或在 Actions 页面手动 `Run workflow`。
  Push to `master`/`main`, a pull request, or a manual `Run workflow` from the Actions page.
- 产物 / Artifacts：每个架构（`x86_64`、`aarch64`）产出 `apt-<arch>-musl` 与 `apt-<arch>-glibc`。
  For each architecture (`x86_64`, `aarch64`) it produces `apt-<arch>-musl` and `apt-<arch>-glibc`.
- 发布 / Release：推送形如 `v1.0.0` 的 tag 会自动创建 Release 并附上全部二进制。
  Pushing a tag like `v1.0.0` automatically creates a Release with all binaries attached.
- 校验 / Verify：工作流会 `file` + `ldd` 确认是静态链接并打印 `--version`。
  The workflow runs `file` + `ldd` to confirm static linking and prints `--version`.

---

## 注意事项 / Notes

1. **不提权 / No privilege escalation.**
   程序不会自动 `sudo`。装到 `/usr/bin/apt` 后请用 `sudo apt install ...` 执行需要 root 的操作；直接以普通用户运行会由后端报权限错误——这与直接运行后端命令一致。
   The program never calls `sudo`. After installing it as `/usr/bin/apt`, prefix privileged ops with `sudo apt ...`; running as a normal user without sudo lets the backend report the permission error, exactly as running the backend directly would.

2. **版本锁定 / Version pinning (`pkg=version`).**
   dnf/yum/xbps 用 `name-version`；zypper/apk 用 `name=version`；emerge 用 `=name-version`；其余后端（pacman/slackpkg/urpm/nix-env/prt-get/guix/pkg/pkgman/cards/opkg/eopkg）无此写法，会去掉版本号并打印 `W:` 告警，改为安装最新版。
   dnf/yum/xbps use `name-version`; zypper/apk use `name=version`; emerge uses `=name-version`. The remaining backends have no equivalent form: the version is stripped, a `W:` warning is printed, and the latest version is installed.

3. **不支持的操作 / Unsupported operations.**
   表中标为不支持（模板为 `{0}`）的操作不会臆造命令，而是打印 `W:`/`E:` 警告或退回等价操作。
   Operations marked unsupported (empty `{0}` templates) never fabricate a command; they print a `W:`/`E:` warning or fall back to an equivalent.

4. **特殊"还原" / Special reproductions.**
   - pacman 没有单条 autoremove 命令，改为"列出孤儿包再删除"（`pacman -Qdtq` → `pacman -R`）。
     pacman has no single autoremove command, so it is reproduced as list-orphans-then-remove.
   - emerge 的"已装列表"扫描 `/var/db/pkg`；slackpkg 的扫描 `/var/log/packages`。
     emerge's installed list scans `/var/db/pkg`; slackpkg's scans `/var/log/packages`.
   - apk 无 autoremove 概念，直接告警不做。
     apk has no autoremove concept; it just warns and does nothing.

5. **stdout/stderr 与退出码 / stdout, stderr and exit codes.**
   本工具只负责转发与执行后端命令，其输出与退出码原样传递。
   The tool only dispatches and executes the backend command; its output and exit code are forwarded as-is.

6. **构建与安装 / Build & Install.**

   ```sh
   make                    # 推荐 / recommended
   # 或等价地 / or equivalently:
   gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   # musl 更干净的静态链接 / cleaner static link with musl:
   musl-gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   ```

   安装到非 apt 系统 / Install onto a non-apt system:

   ```sh
   [ -e /usr/bin/dpkg ] && { echo "Debian-family detected, abort."; exit 1; }
   sudo install -m 0755 apt /usr/bin/apt
   sudo ln -sf apt /usr/bin/apt-get
   sudo ln -sf apt /usr/bin/apt-cache
   ```

7. **参考 / References.**
   映射规则参考 / Mapping rules reference: `icy/pacapt`、`Itai-Nelken/aptpac`、`rami3l/pacaptr`、`sigoden/upt`（逐操作权威表见 `reference/upt/src/vendor.rs`）。源码克隆在 `reference/`。
   Sources cloned under `reference/`; the authoritative per-operation table is `reference/upt/src/vendor.rs`.
