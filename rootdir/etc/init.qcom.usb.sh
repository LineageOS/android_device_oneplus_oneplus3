#!/vendor/bin/sh
# Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#

# Set platform variables
if [ -f /sys/devices/soc0/hw_platform ]; then
    soc_hwplatform=`cat /sys/devices/soc0/hw_platform` 2> /dev/null
else
    soc_hwplatform=`cat /sys/devices/system/soc/soc0/hw_platform` 2> /dev/null
fi

if [ -f /sys/devices/soc0/machine ]; then
    soc_machine=`cat /sys/devices/soc0/machine` 2> /dev/null
else
    soc_machine=`cat /sys/devices/system/soc/soc0/machine` 2> /dev/null
fi

#
# Check ESOC for external MDM
#
# Note: currently only a single MDM is supported
#
if [ -d /sys/bus/esoc/devices ]; then
for f in /sys/bus/esoc/devices/*; do
    if [ -d $f ]; then
    if [ `grep -e "^MDM" -e "^SDX" $f/esoc_name` ]; then
            esoc_link=`cat $f/esoc_link`
            break
        fi
    fi
done
fi

target=`getprop ro.board.platform`

# soc_ids for 8937
if [ -f /sys/devices/soc0/soc_id ]; then
	soc_id=`cat /sys/devices/soc0/soc_id`
else
	soc_id=`cat /sys/devices/system/soc/soc0/id`
fi

if [ -f /sys/class/android_usb/f_mass_storage/lun/nofua ]; then
	echo 1  > /sys/class/android_usb/f_mass_storage/lun/nofua
fi

#
# Override USB default composition
#
# If USB persist config not set, set default configuration
if [ "$(getprop persist.vendor.usb.config)" == "" -a \
	"$(getprop init.svc.vendor.usb-gadget-hal-1-0)" != "running" ]; then
      if [ "$esoc_link" != "" ]; then
	  setprop persist.vendor.usb.config diag,diag_mdm,qdss,qdss_mdm,serial_cdev,dpl,rmnet,adb
      else
	  case "$(getprop ro.baseband)" in
	      "apq")
	          setprop persist.vendor.usb.config diag,adb
	      ;;
	      *)
	      case "$soc_hwplatform" in
	          "Dragon" | "SBC")
	              setprop persist.vendor.usb.config diag,adb
	          ;;
                  *)
		  soc_machine=${soc_machine:0:3}
		  case "$soc_machine" in
		    "SDA")
	              setprop persist.vendor.usb.config diag,adb
		    ;;
		    *)
	            case "$target" in
	              "msm8996")
	                  setprop persist.vendor.usb.config diag,serial_cdev,serial_tty,rmnet_ipa,mass_storage,adb
		      ;;
	              "msm8909")
		          setprop persist.vendor.usb.config diag,serial_smd,rmnet_qti_bam,adb
		      ;;
	              "msm8937")
			    if [ -d /config/usb_gadget ]; then
				       setprop persist.vendor.usb.config diag,serial_cdev,rmnet,dpl,adb
			    else
			               case "$soc_id" in
				               "313" | "320")
				                  setprop persist.vendor.usb.config diag,serial_smd,rmnet_ipa,adb
				               ;;
				               *)
				                  setprop persist.vendor.usb.config diag,serial_smd,rmnet_qti_bam,adb
				               ;;
			               esac
			    fi
		      ;;
	              "msm8953")
			      if [ -d /config/usb_gadget ]; then
				      setprop persist.vendor.usb.config diag,serial_cdev,rmnet,dpl,adb
			      else
				      setprop persist.vendor.usb.config diag,serial_smd,rmnet_ipa,adb
			      fi
		      ;;
	              "msm8998" | "sdm660" | "apq8098_latv")
		          setprop persist.vendor.usb.config diag,serial_cdev,rmnet,adb
		      ;;
	              "sdm845" | "sdm710")
		          setprop persist.vendor.usb.config diag,serial_cdev,rmnet,dpl,adb
		      ;;
	              "msmnile" | "talos")
			  setprop persist.vendor.usb.config diag,serial_cdev,rmnet,dpl,qdss,adb
		      ;;
	              *)
		          setprop persist.vendor.usb.config diag,adb
		      ;;
                    esac
		    ;;
		  esac
	          ;;
	      esac
	      ;;
	  esac
      fi
fi

# set rndis transport to BAM2BAM_IPA for 8920 and 8940
if [ "$target" == "msm8937" ]; then
	if [ ! -d /config/usb_gadget ]; then
	   case "$soc_id" in
		"313" | "320")
		   echo BAM2BAM_IPA > /sys/class/android_usb/android0/f_rndis_qc/rndis_transports
		;;
		*)
		;;
	   esac
	fi
fi

# set device mode notification to USB driver for SA8150 Auto ADP
product=`getprop ro.build.product`

case "$product" in
	"msmnile_au")
	echo peripheral > /sys/bus/platform/devices/a600000.ssusb/mode
         ;;
	*)
	;;
esac

# check configfs is mounted or not
if [ -d /config/usb_gadget ]; then
	# Chip-serial is used for unique MSM identification in Product string
	msm_serial=`cat /sys/devices/soc0/serial_number`;
	msm_serial_hex=`printf %08X $msm_serial`
	machine_type=`cat /sys/devices/soc0/machine`
	product_string="$machine_type-$soc_hwplatform _SN:$msm_serial_hex"
	echo "$product_string" > /config/usb_gadget/g1/strings/0x409/product

	# ADB requires valid iSerialNumber; if ro.serialno is missing, use dummy
	serialnumber=`cat /config/usb_gadget/g1/strings/0x409/serialnumber` 2> /dev/null
	if [ "$serialnumber" == "" ]; then
		serialno=1234567
		echo $serialno > /config/usb_gadget/g1/strings/0x409/serialnumber
	fi
fi

#
# Initialize RNDIS Diag option. If unset, set it to 'none'.
#
diag_extra=`getprop persist.vendor.usb.config.extra`
if [ "$diag_extra" == "" ]; then
	setprop persist.vendor.usb.config.extra none
fi

# enable rps cpus on msm8937 target
setprop vendor.usb.rps_mask 0
case "$soc_id" in
	"294" | "295" | "353" | "354")
		setprop vendor.usb.rps_mask 40
	;;
esac

#
# Initialize UVC conifguration.
#
if [ -d /config/usb_gadget/g1/functions/uvc.0 ]; then
	cd /config/usb_gadget/g1/functions/uvc.0

	echo 3072 > streaming_maxpacket
	echo 1 > streaming_maxburst
	mkdir control/header/h
	ln -s control/header/h control/class/fs/
	ln -s control/header/h control/class/ss

	mkdir -p streaming/uncompressed/u/360p
	echo "666666\n1000000\n5000000\n" > streaming/uncompressed/u/360p/dwFrameInterval

	mkdir -p streaming/uncompressed/u/720p
	echo 1280 > streaming/uncompressed/u/720p/wWidth
	echo 720 > streaming/uncompressed/u/720p/wWidth
	echo 29491200 > streaming/uncompressed/u/720p/dwMinBitRate
	echo 29491200 > streaming/uncompressed/u/720p/dwMaxBitRate
	echo 1843200 > streaming/uncompressed/u/720p/dwMaxVideoFrameBufferSize
	echo 5000000 > streaming/uncompressed/u/720p/dwDefaultFrameInterval
	echo "5000000\n" > streaming/uncompressed/u/720p/dwFrameInterval

	mkdir -p streaming/mjpeg/m/360p
	echo "666666\n1000000\n5000000\n" > streaming/mjpeg/m/360p/dwFrameInterval

	mkdir -p streaming/mjpeg/m/720p
	echo 1280 > streaming/mjpeg/m/720p/wWidth
	echo 720 > streaming/mjpeg/m/720p/wWidth
	echo 29491200 > streaming/mjpeg/m/720p/dwMinBitRate
	echo 29491200 > streaming/mjpeg/m/720p/dwMaxBitRate
	echo 1843200 > streaming/mjpeg/m/720p/dwMaxVideoFrameBufferSize
	echo 5000000 > streaming/mjpeg/m/720p/dwDefaultFrameInterval
	echo "5000000\n" > streaming/mjpeg/m/720p/dwFrameInterval

	echo 0x04 > /config/usb_gadget/g1/functions/uvc.0/streaming/mjpeg/m/bmaControls

	mkdir -p streaming/h264/h/960p
	echo 1920 > streaming/h264/h/960p/wWidth
	echo 960 > streaming/h264/h/960p/wWidth
	echo 40 > streaming/h264/h/960p/bLevelIDC
	echo "333667\n" > streaming/h264/h/960p/dwFrameInterval

	mkdir -p streaming/h264/h/1920p
	echo "333667\n" > streaming/h264/h/1920p/dwFrameInterval

	mkdir streaming/header/h
	ln -s streaming/uncompressed/u streaming/header/h
	ln -s streaming/mjpeg/m streaming/header/h
	ln -s streaming/h264/h streaming/header/h
	ln -s streaming/header/h streaming/class/fs/
	ln -s streaming/header/h streaming/class/hs/
	ln -s streaming/header/h streaming/class/ss/
fi
