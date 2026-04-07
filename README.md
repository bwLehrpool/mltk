# mltk - Multi-Linux ToolKit

`mltk` is a bash-based build system designed for customizing Linux installations. Its primary goal is to create Linux systems that are booted over the network in a non-persistent fashion (e.g., using copy-on-write to RAM or a temporary partition).

While technically distro-agnostic, the current focus and most tested environment is Debian.

## Core Concepts

### Targets
A **target** defines a specific set of modules to be built together. Targets are located in `core/targets/` and are typically structured as a directory containing symlinks to the modules it includes. This allows for easy composition of different system configurations.

### Modules
A **module** is an encapsulated unit of build or install logic. It defines what needs to be fetched, built, or copied into the final system. Modules are located in `core/modules/` (global modules) or can be specific to a target.

### Operation Modes
`mltk` supports two primary modes:

*   **Legacy mode (MiniLinux)**: The default mode. It aims to create a fresh, lean rootfs from scratch (e.g., for packaging as a squashfs). This approach is older and newer modules may not be fully tested with it.
*   **Install mode (MaxiLinux)**: Triggered by passing `-i` to `mltk`. This mode assumes the build system is running inside a Debian-based installation that it will then modify. This is the currently preferred mode for developing and using modules, resulting in a larger but more feature-rich system.

---

## Module Structure

A module directory typically contains:

*   `module.conf`: Main configuration file (bash script).
*   `module.conf.<distro>`: (Optional) Distro-specific overrides (e.g., `module.conf.debian`).
*   `module.build`: (Optional) Bash script containing build and lifecycle hooks.
*   `data/`: (Optional) Static files to be included in the final system.

### Configuration Variables (`module.conf`)
Variables used by the build system to resolve dependencies and copy files must start with the `REQUIRED_` prefix:

*   `REQUIRED_MODULES`: List of other modules this module depends on.
*   `REQUIRED_BINARIES`: Binaries to copy to the target. `mltk` automatically resolves and copies their dynamic library dependencies.
*   `REQUIRED_LIBRARIES`: Specific libraries to copy.
*   `REQUIRED_FILES`: Specific files to copy.
*   `REQUIRED_DIRECTORIES`: Directories to copy recursively.
*   `REQUIRED_SYSTEM_FILES`: Files to copy from the host system. (no-op in install mode)
*   `REQUIRED_INSTALLED_PACKAGES`: Packages that must be installed on the host system to build this module.
*   `REQUIRED_CONTENT_PACKAGES`: Packages whose contents (all files) should be extracted and included in the target system. (no-op in install mode)
*   `REQUIRED_GIT`: Git repositories to clone (see `autoclone`).

### Build Hooks (`module.build`)
You can define the following bash functions in `module.build` to control the build process:

*   `module_init`: Runs before package installation. Useful for adding apt repositories or GPG keys.
*   `pre_exec`: Runs before fetching source.
*   `fetch_source`: Logic to download or clone source code (e.g., using `autoclone` or `download`).
*   `build`: The main build logic. Use `$MODULE_BUILD_DIR` as your installation prefix.
*   `post_copy`: Runs after all files (from `data/`, `REQUIRED_FILES`, etc.) have been copied to the final target rootfs.

---

## Predefined Variables

The build system provides several variables to your module scripts:

*   `ROOT_DIR`: Absolute path to the `mltk` project root.
*   `MODULE_DIR`: Path to the current module's directory.
*   `MODULE_WORK_DIR`: A temporary working directory for the module (e.g., for extracting source).
*   `MODULE_BUILD_DIR`: Where your module should "install" its files during the `build` hook.
*   `TARGET_BUILD_DIR`: The absolute path to the final rootfs of the target being built.
*   `CPU_CORES`: Number of CPU cores available (useful for `make -j`).
*   `ARCH_TRIPLET`: The architecture triplet (e.g., `x86_64-linux-gnu`).
*   `ARCH_LIB_DIR`: The architecture-specific library directory (e.g., `lib/x86_64-linux-gnu`).

---

## Provided Helpers

`mltk` includes several helper functions (mostly defined in `core/includes/`):

### `chroot_run`
Executes a block of code (via stdin) inside a chroot of the specified directory. It uses `overlayfs` to present the host's root filesystem as the base, making it look like a full system while only persisting changes made to the target directory.

```bash
chroot_run "${MODULE_BUILD_DIR}" <<-EOF
    apt-get update
    apt-get install -y my-package
EOF
```

### `autoclone`
Clones repositories defined in `REQUIRED_GIT`.
Format: `URL[|branch][||commit-ish]`
```bash
REQUIRED_GIT="https://github.com/example/repo.git|main"
# In module.build:
fetch_source() {
    autoclone
}
```

### `download` & `download_untar`
Helpers for fetching files via `wget`.
*   `download <url> [filename]`
*   `download_untar <url> <destination_dir>`

### Other Helpers
*   `cde <directory>`: Change directory and exit with an error if it fails.
*   `tarcopy <source> <destination>`: Efficiently copy files using `tar` (preserving permissions and links).

### Logging
*   `pinfo "message"`: Informational output.
*   `pwarning "message"`: Warning output.
*   `perror "message"`: Error output (terminates the build).

---

## Caveats & Tips

*   **Distro Focus**: While `mltk` aims to be distro-agnostic, many modules rely on Debian-specific paths or `apt`.
*   **Chroot Environment**: `chroot_run` provides a powerful way to install software as if you were on a live system, but remember that it's an overlay. Be careful with mount points and hardware access.
*   **Lean vs. Full**: Legacy-mode (MiniLinux) is designed to be minimal. Avoid adding large dependencies unless necessary. Install-mode (MaxiLinux) is more forgiving but results in larger images.
*   **Cleanliness**: Always use `$MODULE_BUILD_DIR` for files that should end up in the final system, and `$MODULE_WORK_DIR` for temporary build artifacts.
