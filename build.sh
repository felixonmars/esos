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

# create the version id with git commit id
function create_version_id()
{
	mk_info "create the verion id ..."

	pushd ${TOP_DIR}
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
	popd
}

function config_sdk()
{
	mk_info "prepare to config esos sdk ..."

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
}

function build_kernel()
{
	# build dtb
	source ${ESOS_BASE_DEFCONF}
	cd ${BSP_DIR}/platform/${TARGET_CHIP}/${TARGET_BOARD}/dts/
	make
	if [ $? -ne 0 ]; then
		mk_error "Failed to build dtb"
		cd -
		return 1
	fi
	cp ./*.dtb ../../
	make clean
	cd -

	# generate version id
	create_version_id

	# build src
	source ${ESOS_BASE_DEFCONF}
	# Export variables for Python scripts
	export TARGET_CHIP TARGET_BOARD TARGET_ENTRY_POINT TARGET_DEFCONFIG
	cd ${BSP_DIR}
	scons --useconfig=.config
	if [ $? -ne 0 ]; then
		mk_error "Failed to load config"
		cd -
		return 1
	fi
	scons
	if [ $? -ne 0 ]; then
		mk_error "Failed to build esos"
		cd -
		return 1
	fi
	cd -
	return 0
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
	if [ $? -ne 0 ]; then
		mk_error "Failed to build dtb for ${core_name}"
		cd -
		return 1
	fi
	cp ./*.dtb ../../
	make clean
	cd -

	# generate version id
	create_version_id

	# Build src
	source ${ESOS_BASE_DEFCONF}
	# Export variables for Python scripts
	export TARGET_CHIP TARGET_BOARD TARGET_ENTRY_POINT TARGET_DEFCONFIG
	cd ${BSP_DIR}
	scons --useconfig=.config
	if [ $? -ne 0 ]; then
		mk_error "Failed to load config for ${core_name}"
		cd -
		return 1
	fi
	scons
	if [ $? -ne 0 ]; then
		mk_error "Failed to build ${core_name}"
		cd -
		return 1
	fi
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
	else
		mk_error "Build output file not found: ${BSP_DIR}/rtthread-rt24.elf"
		return 1
	fi
	if [ -f "${BSP_DIR}/rtthread.bin" ]; then
		mv "${BSP_DIR}/rtthread.bin" "${BSP_DIR}/rtthread-${output_suffix}.bin"
		mk_info "Output: ${BSP_DIR}/rtthread-${output_suffix}.bin"
	fi
	return 0
}

function build_all_cores()
{
	local sign_mode="$1"
	local key_dir="$2"

	mk_info "Building all K3 cores (k3_core0 and k3_core1)..."

	# Clean previous builds
	clean_kernel

	# Build core0
	build_single_core "k3_core0" "core0"
	if [ $? -ne 0 ]; then
		mk_error "Failed to build core0"
		return 1
	fi

	# Clean for next build
	cd ${BSP_DIR}
	scons -c
	cd -

	# Build core1
	build_single_core "k3_core1" "core1"
	if [ $? -ne 0 ]; then
		mk_error "Failed to build core1"
		return 1
	fi

	# Create ITB package (signed or unsigned)
	if [ "x${sign_mode}" = "xsign" ]; then
		create_esos_itb "sign" "${key_dir}"
	else
		create_esos_itb
	fi

	mk_info "All cores built successfully!"
	mk_info "Generated files:"
	mk_info "  - ${BSP_DIR}/k3_os0_rcpu.elf"
	mk_info "  - ${BSP_DIR}/k3_os1_rcpu.elf"
	mk_info "  - ${BSP_DIR}/esos.itb"
	return 0
}

function create_esos_itb()
{
	local sign_mode="$1"
	local key_dir="$2"

	# Always output as esos.itb regardless of signing
	local itb_file="esos.itb"

	if [ "x${sign_mode}" = "xsign" ]; then
		mk_info "Creating signed ESOS ITB package..."
		local its_file="esos_sign.its"
	else
		mk_info "Creating ESOS ITB package..."
		local its_file="esos.its"
	fi

	# Use ITS template from top directory
	local its_path="${TOP_DIR}/${its_file}"
	if [ ! -f "${its_path}" ]; then
		mk_error "ITS template not found: ${its_path}"
		return 1
	fi
	mk_info "Using ITS template: ${its_path}"

	# Check if mkimage is available
	if ! command -v mkimage >/dev/null 2>&1; then
		mk_warn "mkimage not found, ITB not created. Please install u-boot-tools."
		return 1
	fi

	# Handle signing parameters before changing directory
	if [ "x${sign_mode}" = "xsign" ]; then
		if [ -z "${key_dir}" ]; then
			mk_error "Key directory not specified"
			mk_error "Please specify key directory: ./build.sh sign_with_key <key_dir>"
			return 1
		fi

		# Convert to absolute path if needed (before cd)
		if [[ "${key_dir}" != /* ]]; then
			local abs_key_dir="$(cd "${key_dir}" 2>/dev/null && pwd)"
			if [ -z "${abs_key_dir}" ]; then
				mk_error "Key directory does not exist: ${key_dir}"
				return 1
			fi
			key_dir="${abs_key_dir}"
		fi

		if [ ! -d "${key_dir}" ]; then
			mk_error "Key directory not found: ${key_dir}"
			mk_error "Please create keys using: openssl genpkey -algorithm RSA -out kernel_key_prv.key ..."
			return 1
		fi
	fi

	# Generate ITB using mkimage (run from TOP_DIR for correct relative paths in ITS)
	cd ${TOP_DIR}

	if [ "x${sign_mode}" = "xsign" ]; then

		# Create temporary DTB for public key
		UBOOT_DIR="${TOP_DIR}/../uboot-2022.10"
		if [ -f "${UBOOT_DIR}/u-boot.dtb" ]; then
			cp "${UBOOT_DIR}/u-boot.dtb" ${BSP_DIR}/u-boot-esos-sign.dtb
		else
			# Create empty DTB if u-boot.dtb doesn't exist
			printf "/dts-v1/;\n/ {\n};" > ${BSP_DIR}/u-boot-esos-sign.dts
			dtc -I dts -O dtb -o ${BSP_DIR}/u-boot-esos-sign.dtb ${BSP_DIR}/u-boot-esos-sign.dts 2>/dev/null
		fi

		mkimage -f ${its_path} -K ${BSP_DIR}/u-boot-esos-sign.dtb -k "${key_dir}" -r ${BSP_DIR}/${itb_file}
		mk_info "Signed ITB created: ${BSP_DIR}/${itb_file}"
	else
		mkimage -f ${its_path} ${BSP_DIR}/${itb_file}
		mk_info "ITB created: ${BSP_DIR}/${itb_file}"
	fi

	# Copy ITB to output directory
	OUTPUT_DIR="${TOP_DIR}/../output/esos"
	mkdir -p "${OUTPUT_DIR}"
	cp ${BSP_DIR}/${itb_file} "${OUTPUT_DIR}/"
	cp ${BSP_DIR}/${itb_file} "${TOP_DIR}/../output/"
	mk_info "ITB copied to: ${OUTPUT_DIR}/${itb_file}"

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
}

# execute some command without configuration
if [ "x$1" = "xhelp" ]; then
	build_usage
	exit 0
elif [ "x$1" = "xconfig" ]; then
	config_sdk
	exit 0
elif [ "x$1" = "xsign" ]; then
	# Build with signature using default key directory
	if [ -f "${ESOS_BASE_DEFCONF}" ]; then
		source ${ESOS_BASE_DEFCONF}
		if [ "${TARGET_BOARD}" = "k3_all_cores" ]; then
			build_all_cores "sign"
		else
			mk_error "Sign mode only supported for k3_all_cores target"
			exit 1
		fi
	else
		mk_error "Please run './build.sh config' first"
		exit 1
	fi
	exit 0
elif [ "x$1" = "xsign_with_key" ]; then
	# Build with signature using specified key directory
	if [ -z "$2" ]; then
		mk_error "Please specify key directory: ./build.sh sign_with_key <key_dir>"
		exit 1
	fi
	if [ -f "${ESOS_BASE_DEFCONF}" ]; then
		source ${ESOS_BASE_DEFCONF}
		if [ "${TARGET_BOARD}" = "k3_all_cores" ]; then
			build_all_cores "sign" "$2"
		else
			mk_error "Sign mode only supported for k3_all_cores target"
			exit 1
		fi
	else
		mk_error "Please run './build.sh config' first"
		exit 1
	fi
	exit 0
elif [ "x$1" = "x" ]; then
	# Check if we need to build all cores
	if [ -f "${ESOS_BASE_DEFCONF}" ]; then
		source ${ESOS_BASE_DEFCONF}
		if [ "${TARGET_BOARD}" = "k3_all_cores" ]; then
			build_all_cores
			exit $?
		else
			build_kernel
			exit $?
		fi
	else
		build_kernel
		exit $?
	fi
elif [ "x$1" = "xclean" ]; then
	clean_kernel
	exit 0
fi
