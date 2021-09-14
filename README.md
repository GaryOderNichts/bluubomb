# BluuBomb

Exploits the Wii U's bluetooth stack to gain IOSU kernel access via bluetooth.

For a more detailed write-up see [WRITEUP.md](https://github.com/GaryOderNichts/bluubomb/blob/master/WRITEUP.md).  

Not to be confused with [BlueBomb](https://github.com/Fullmetal5/bluebomb) for the Wii and Wii Mini.  

## Requirements
- A Wii U which is able to pair a Wii Remote
- A PC with bluetooth
- A PC or VM running a version of Linux which is able to run the custom build of BlueZ  

## How to use
1. Run `sudo apt install build-essential libbluetooth-dev libglib2.0-dev libdbus-1-dev git` to install the required dependencies.
1. Run `git clone https://github.com/rnconrad/WiimoteEmulator && cd WiimoteEmulator`.
1. Run `source ./build-custom.sh` to build BlueZ.  
Don't worry if building the emulator itself fails due to missing SDL headers. Just continue with the next steps.  
1. Stop the already running bluetooth service `sudo systemctl disable --now bluetooth`
1. Run the custom built bluetoothd `sudo ./bluez-4.101/dist/sbin/bluetoothd -d -n`
1. Download the `bluubomb` binary and the `sd_kernels.zip` from the [releases page](https://github.com/GaryOderNichts/bluubomb/releases).  
Copy a kernel binary of your choice from the `sd_kernels.zip` to the root of your SD Card and rename it to `bluu_kern.bin`.  
Take a look at [Kernel binaries](#kernel-binaries) for more information.
1. Power on the Wii U, insert your SD Card and press the sync button.
1. Open a new terminal and make the bluubomb file executable by running `chmod +x bluubomb`
1. Run `sudo ./bluubomb` and wait for the pairing process to complete.  
This might take a minute.  
If you get a warning about Simple Pairing mode read [the Simple Pairing mode section below](#simple-pairing-mode). 

Write down the Wii U's bluetooth device address that's displayed after the pairing is complete.  
You can now run `sudo ./bluubomb <bdaddr here>` to connect directly to the Wii U and skip the pairing process.

## Kernel binaries

### loadrpx.bin
Launches a launch.rpx from the root of your SD card on the next application launch.

### regionfree.bin
Applies IOSU patches to temporarily remove region restrictions.  
This should be helpful if you've locked yourself out of your applications due to permanent region modifications.

### wupserver.bin
Launches a wupserver instance directly after using bluubomb.  
This gets you full system access remotely via [wupclient](https://github.com/dimok789/mocha/blob/master/ios_mcp/wupclient.py) (replace the IP in line 29 with the one of your Wii U).  
This works without having to leave the controller pairing screen.

## Simple Pairing mode

On some devices the simple pairing mode can't be disabled by bluubomb.  
You can check the current Simple Pairing mode by running `hciconfig hci0 sspmode`.  
Make sure it says `Simple Pairing mode: Disabled`.  
If not run `sudo hciconfig hci0 sspmode disabled` and `sudo hciconfig hci0 reset`.  
Then check the mode again.  

## Building

To build you need to have gcc and devkitARM installed.  
Then run `make`.

## Credits
- GaryOderNichts - bluubomb  
- rnconrad for the [WiimoteEmulator](https://github.com/rnconrad/WiimoteEmulator)  
- dimok789 and everyone else who made [mocha](https://github.com/dimok789/mocha) possible  
