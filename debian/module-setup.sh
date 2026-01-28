#!/bin/bash
# 99esos dracut module - 带调试输出，确保 rt24_os0_rcpu.elf/rt24_os1_rcpu.elf 被安装到 ramdisk 的 /lib/firmware

check() {
    # 返回 0 表示模块可用
    return 0
}

depends() {
    # 若依赖其它 dracut 模块，echo 出名字；否则空
    echo ""
    return 0
}

install() {
    # 调试输出，dracut --debug 会显示
    echo "99esos: install() called, moddir=${moddir}" >&2

    local esos_files=("rt24_os0_rcpu.elf" "rt24_os1_rcpu.elf")
    local found_any=0

    for f in "${esos_files[@]}"; do
        if [ -f "${moddir}/${f}" ]; then
            echo "99esos: found ${moddir}/${f}, installing to /lib/firmware/${f}" >&2
            inst_simple "${moddir}/${f}" "/lib/firmware/${f}"
            found_any=1
        elif [ -f "/usr/lib/riscv64-linux-gnu/esos/${f}" ]; then
            echo "99esos: found /usr/lib/riscv64-linux-gnu/esos/${f} on host, installing to /lib/firmware/${f}" >&2
            inst_simple "/usr/lib/riscv64-linux-gnu/esos/${f}" "/lib/firmware/${f}"
            found_any=1
        else
            echo "99esos: WARNING: ${f} not found in module dir or /usr/lib/riscv64-linux-gnu/esos" >&2
        fi
    done

    if [ "${found_any}" -eq 0 ]; then
        echo "99esos: WARNING: no ESOS ELF files installed" >&2
    fi

    return 0
}