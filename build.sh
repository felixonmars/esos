#!/bin/bash

export TOP_DIR=`pwd`
export CPU_DIR="${TOP_DIR}/libcpu/risc-v/spacemit"
export BSP_DIR="${TOP_DIR}/bsp/spacemit"
export BOARD_DIR="${BSP_DIR}/platform"
export ESOS_BASE_DEFCONF="${BSP_DIR}/.esos.config"
export ESOS_DEFCONF="${BSP_DIR}/.config"

TARGET_CHIP=
TARGET_BOARD=
TARGET_ENTRY_POINT=
TARGET_DEFCONFIG=

function mk_error()
{
	echo -e "\033[40;31mERROR: $*\033[0m"
}

function mk_warn()
{
	echo -e "\033[40;33;1mmWARN: $*\033[0m"
}

function mk_info()
{
	echo -e "\033[40;37mINFO: $*\033[0m"
}

function select_chip()
{
	count=0

	printf "All valid soc chips:\n"

	for chip in $(cd $CPU_DIR/; find -mindepth 1 -maxdepth 1 -type d |sort); do
		if [ `basename $CPU_DIR/$TARGET_CHIP/$chip` != ".git" ] ; then
			chips[$count]=`basename $CPU_DIR/$chip`
			printf "	$count: ${chips[$count]}\n"
			let count=$count+1
		fi
	done

	if [ "$count" -gt 0 ] ; then
		while true; do
			read -p "Please select a chip:"
			RES=`expr match $REPLY "[0-9][0-9]*$"`
			if [ "$RES" -le 0 ]; then
				printf "please use index number\n"
				continue
			fi
			if [ "$REPLY" -ge $count ] || [ "$REPLY" -lt "0" ]; then
				mk_error "input is invalid!"
				continue
			fi
			break
		done

		TARGET_CHIP=${chips[$REPLY]}
		return 0
	else
		mk_error "No valid chip!"
		return 1
	fi
}

function select_board()
{
	count=0

	printf "All valid boards:\n"

	for board in $(cd $BOARD_DIR/$TARGET_CHIP; find -mindepth 1 -maxdepth 1 -type d |grep -v default|sort); do
		if [ `basename $BOARD_DIR/$TARGET_CHIP/$board` != ".git" ] ; then
			boards[$count]=`basename $BOARD_DIR/$TARGET_CHIP/$board`
			printf "\t$count: ${boards[$count]}\n"
			let count=$count+1
		fi
	done

	if [ "$TARGET_CHIP" = "rt24" ]; then
		boards[$count]="k3_all_cores"
		printf "\t$count: ${boards[$count]} (build both k3_core0 and k3_core1)\n"
		let count=$count+1
	fi

	if [ "$count" -gt 0 ] ; then
		while true; do
			read -p "Please select a board:"
			RES=`expr match $REPLY "[0-9][0-9]*$"`
			if [ "$RES" -le 0 ]; then
				printf "please use index number\n"
				continue
			fi
			if [ "$REPLY" -ge $count ] || [ "$REPLY" -lt "0" ]; then
				printf "input is invalid!\n"
				continue
			fi
			break
		done

		TARGET_BOARD=${boards[$REPLY]}
		return 0
	else
		mk_error "No valid board!"
		return 1
	fi
}

function select_entry_point()
{
	if [ "x${TARGET_CHIP}_${TARGET_BOARD}" = "xn308_k1-x" ]; then
		TARGET_ENTRY_POINT=0x30300000
	elif [ "x${TARGET_CHIP}_${TARGET_BOARD}" = "xrt24_k3_core0" ]; then
 		TARGET_ENTRY_POINT=0x100200000
	elif [ "x${TARGET_CHIP}_${TARGET_BOARD}" = "xrt24_k3_core1" ]; then
		TARGET_ENTRY_POINT=0x100804000
	elif [ "x${TARGET_CHIP}_${TARGET_BOARD}" = "xrt24_k3_all_cores" ]; then
		# Skip entry point setting for all_cores, will be handled in build phase
		return 0
	else
		mk_error "No valid entry point!"
		return 1
	fi
}

# build script usage helper
function build_usage()
{
	CMD_PROMPT="./build.sh"
	mk_info "usage of build script is as follows:
	'$CMD_PROMPT config'                 set the SDK configuration
	'$CMD_PROMPT all'                    build all component
	'$CMD_PROMPT'                        build all component (supports k3_all_cores option)
	'$CMD_PROMPT clean'                  clean the kernel\n"
}

# show target configuration
function show_target_config()
{
	echo
	mk_info "target configuration is as follows:"
	mk_info "-------------------------------------------------------------------------"
	cat ${ESOS_BASE_DEFCONF}
	mk_info "-------------------------------------------------------------------------"
}

function config_sdk()
{
	mk_info "prepare to config sdk ..."

	# delete the old configuration script
	rm -rf ${ESOS_DEFCONF}
	rm -rf ${ESOS_BASE_DEFCONF}

	select_chip
	select_board
	select_entry_point

	TARGET_DEFCONFIG=${TARGET_CHIP}_${TARGET_BOARD}_defconfig

	# check if the build.cfg is full configured
	if [ "x${TARGET_CHIP}" = "x" ]; then
		mk_error "TARGET_CHIP is not configured!!!"
	fi

	if [ "x${TARGET_BOARD}" = "x" ]; then
		mk_error "TARGET_BOARD is not configured!!!"
	fi

	echo "export TARGET_CHIP=${TARGET_CHIP}" >> ${ESOS_BASE_DEFCONF}
	echo "export TARGET_BOARD=${TARGET_BOARD}" >> ${ESOS_BASE_DEFCONF}
	echo "export TARGET_DEFCONFIG=${TARGET_DEFCONFIG}" >> ${ESOS_BASE_DEFCONF}
	echo "export TARGET_ENTRY_POINT=${TARGET_ENTRY_POINT}" >> ${ESOS_BASE_DEFCONF}

	source ${ESOS_BASE_DEFCONF}

	# Skip config copy for k3_all_cores, will be handled in build phase
	if [ "${TARGET_BOARD}" != "k3_all_cores" ]; then
		cp ${BOARD_DIR}/${TARGET_CHIP}/${TARGET_BOARD}/${TARGET_DEFCONFIG} ${BSP_DIR}/.config
	fi

	show_target_config

	mk_info "prepare to toolchain ..."

	if [ "x${TARGET_CHIP}" = "xn308" ]; then
		if [ ! -d "${TOP_DIR}/tools/toolchain/gcc" ]; then
			cd ${TOP_DIR}/tools/toolchain/
			tar -jxvf ${TOP_DIR}/tools/toolchain/nuclei_riscv_newlibc_prebuilt_linux64_2022.12.tar.bz2
			cd -
		fi
	elif [ "x${TARGET_CHIP}" = "xrt24" ]; then
		if [ ! -d "${TOP_DIR}/tools/toolchain/spacemit-toolchain-elf-newlib-x86_64-v1.0.9" ]; then
			cd ${TOP_DIR}/tools/toolchain/
			tar -xf ${TOP_DIR}/tools/toolchain/spacemit-toolchain-elf-newlib-x86_64-v1.0.9.tar.xz
			cd -
		fi
	fi

	# create the rtconfig.h, it will be updated
	touch ${TOP_DIR}/bsp/spacemit/rtconfig.h

	mk_info "create the verion id ..."

	cd ${TOP_DIR}
	commit_id=$(git log | head -1)
	version_id=${commit_id: -12}
	__version_id=".verid=\"${TARGET_BOARD}:${version_id}\""
	echo ${__version_id}
	version_id_cfg_file=${TOP_DIR}/bsp/spacemit/platform/version_id_gen.cc
	rm -f ${version_id_cfg_file}
	touch ${version_id_cfg_file}
	echo "struct version_id __versionid spacemit_verid = {" >> ${version_id_cfg_file}
	echo "    ${__version_id}," >> ${version_id_cfg_file}
	echo "};" >> ${version_id_cfg_file}
	cd -
}

function build_kernel()
{
	# build dtb
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}/platform/${TARGET_CHIP}/${TARGET_BOARD}/dts/
	make
	cd -

	# build src 
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}
	scons --useconfig=.config
	scons
	cd -
}

function build_single_core()
{
	local core_name=$1
	local output_suffix=$2

	mk_info "Building ${core_name}..."

	# Set target board for this core
	TARGET_BOARD=${core_name}
	select_entry_point
	TARGET_DEFCONFIG=${TARGET_CHIP}_${TARGET_BOARD}_defconfig

	# Update config files
	echo "export TARGET_CHIP=${TARGET_CHIP}" > ${ESOS_BASE_DEFCONF}
	echo "export TARGET_BOARD=${TARGET_BOARD}" >> ${ESOS_BASE_DEFCONF}
	echo "export TARGET_DEFCONFIG=${TARGET_DEFCONFIG}" >> ${ESOS_BASE_DEFCONF}
	echo "export TARGET_ENTRY_POINT=${TARGET_ENTRY_POINT}" >> ${ESOS_BASE_DEFCONF}

	cp ${BOARD_DIR}/${TARGET_CHIP}/${TARGET_BOARD}/${TARGET_DEFCONFIG} ${BSP_DIR}/.config

	# Build dtb
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}/platform/${TARGET_CHIP}/${TARGET_BOARD}/dts/
	make
	cd -

	# Build src
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}
	scons --useconfig=.config
	scons
	cd -

	# Rename output files to avoid overwriting
	if [ -f "${BSP_DIR}/rtthread-rt24.elf" ]; then
		if [ "${output_suffix}" = "core0" ]; then
			mv "${BSP_DIR}/rtthread-rt24.elf" "${BSP_DIR}/k3_os0_rcpu.elf"
			mk_info "Output: ${BSP_DIR}/k3_os0_rcpu.elf"
		elif [ "${output_suffix}" = "core1" ]; then
			mv "${BSP_DIR}/rtthread-rt24.elf" "${BSP_DIR}/k3_os1_rcpu.elf"
			mk_info "Output: ${BSP_DIR}/k3_os1_rcpu.elf"
		fi
	fi
	if [ -f "${BSP_DIR}/rtthread.bin" ]; then
		mv "${BSP_DIR}/rtthread.bin" "${BSP_DIR}/rtthread-${output_suffix}.bin"
		mk_info "Output: ${BSP_DIR}/rtthread-${output_suffix}.bin"
	fi
}

function build_all_cores()
{
	mk_info "Building all K3 cores (k3_core0 and k3_core1)..."

	# Clean previous builds
	clean_kernel

	# Build core0
	build_single_core "k3_core0" "core0"

	# Clean for next build
	cd ${BSP_DIR}
	scons -c
	cd -

	# Build core1
	build_single_core "k3_core1" "core1"

	# Create ITB package
	create_esos_itb

	mk_info "All cores built successfully!"
	mk_info "Generated files:"
	mk_info "  - ${BSP_DIR}/k3_os0_rcpu.elf"
	mk_info "  - ${BSP_DIR}/k3_os1_rcpu.elf"
	mk_info "  - ${BSP_DIR}/esos.itb"
}

function create_esos_itb()
{
	mk_info "Creating ESOS ITB package..."

	# Copy ITS template to build directory
	if [ -f "${TOP_DIR}/esos.its" ]; then
		cp "${TOP_DIR}/esos.its" "${BSP_DIR}/esos.its"
		mk_info "Using ITS template: ${TOP_DIR}/esos.its"
	else
		mk_error "ITS template not found: ${TOP_DIR}/esos.its"
		return 1
	fi

	# Generate ITB using mkimage
	cd ${BSP_DIR}
	if command -v mkimage >/dev/null 2>&1; then
		mkimage -f esos.its esos.itb
		mk_info "ITB created: ${BSP_DIR}/esos.itb"

		# Copy ITB to output directory
		OUTPUT_DIR="${TOP_DIR}/../output/esos"
		mkdir -p "${OUTPUT_DIR}"
		cp esos.itb "${OUTPUT_DIR}/"
		mk_info "ITB copied to: ${OUTPUT_DIR}/esos.itb"
	else
		mk_warn "mkimage not found, ITB not created. Please install u-boot-tools."
	fi
	cd -
}

function clean_kernel()
{
	# clean src
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}
	touch rtconfig.h
	scons -c
	cd -

	# clean dtb (skip for k3_all_cores virtual target)
	if [ "${TARGET_BOARD}" != "k3_all_cores" ]; then
		cd ${BSP_DIR}/platform/${TARGET_CHIP}/${TARGET_BOARD}/dts/
		make clean
		cd -
	fi
}

# execute some command without configuration
if [ "x$1" = "xhelp" ]; then
	build_usage
	exit 0
elif [ "x$1" = "xconfig" ]; then
	config_sdk
	exit 0
elif [ "x$1" = "x" ]; then
	# Check if we need to build all cores
	if [ -f "${ESOS_BASE_DEFCONF}" ]; then
		source ${ESOS_BASE_DEFCONF}
		if [ "${TARGET_BOARD}" = "k3_all_cores" ]; then
			build_all_cores
		else
			build_kernel
		fi
	else
		build_kernel
	fi
	exit 0
elif [ "x$1" = "xclean" ]; then
	clean_kernel
	exit 0
fi
