# BluuBomb write-up

BluuBomb allows running an IOSU kernel binary by sending data from an emulated Wii Remote.  

## Overview

The Wii U's operating system consists of CafeOS, which runs on the PPC and the IOSU, which itself runs on a chip called the Starbuck. Yeah, for some reason the names of everything on the Wii U are coffee-related.  
While the Wii U has several exploits and entrypoints on the PPC side, the IOSU only has a few exploits and no direct entrypoints.  
To get IOSU code execution you always have to go through the PPC side.  
The IOSU consists of several modules. I was working on reverse engineering some IOSU modules to gain some additional knowledge for a project I was working on.  
The most interesting modules for this write-up are IOS-PAD and IOS-KERNEL.  
IOS-PAD handles most of the controller-related things. It communicates with 2 libraries on the PPC side: vpad.rpl and padscore.rpl.  
I was curious to see how the communication between padscore and IOS-PAD is handled, so I decided to take a look at IOS-PAD and padscore.  
IOS-KERNEL is the kernel of the IOSU (obviously). It has the most permissions in the IOSU and can write to executable memory that is usually not writable.  
The main goal of an IOSU exploit would be to load a custom binary into the kernel.  

## Communication between padscore and IOS-PAD

IOS-PAD uses a resource manager named `/dev/usb/btrm`.  
Padscore will then use ioctl/ioctlv calls to communicate with it.  
Once padscore is loaded it will set up something called `SMD`, which I assume stands for `Simple Message Deque` (?). It uses a shared buffer between the PPC and IOSU and is able to send and receive messages between each other.  
SMD on the PPC side is called `smdPpc` and is part of [coreinit](https://github.com/devkitPro/wut/blob/master/cafe/coreinit.def#L1217), while on the IOSU it's called `smdIop`.  
The buffer is allocated in padscore and its' address is passed to the IOSU using btrm's ioctl call 1.
Once an HID-report is received via Bluetooth it's copied to the stack and passed to the PPC via a `smdIopSendMessage` call.  

## The bug

When the HID-report is received `bta_hh_co_data` is called.  
The function looks something like this:
```c
void bta_hh_co_data(uint8_t dev_handle, void *p_rpt, uint16_t len, uint8_t mode, uint8_t sub_class, uint8_t ctry_code, void *peer_addr, uint8_t app_id)
{
    HidBuffer hid;
...
    if (len == 0) {
        log_printf("BT: [Err] received invalid HID len==0 \n");
    }
    else {
        hid.len = len;
        hid.sub_class = sub_class;
        hid.app_id = app_id;
        hid.dev_handle = dev_handle;
        hid.mode = mode;
        memcpy(hid.data, p_rpt, len);
...
        res = smdIopSendMessage(smdIopIndex, &hid, 0x40);
        if (res == 0) {
            numSucessfulSmdMessages = numSucessfulSmdMessages + 1;
        }
        else {
...
            pad_printf("BT: [Err] SMD send message failed with status %d:%d\n", res);
            numFailedSmdMessages = numFailedSmdMessages + 1;
        }
    }
...
}
```
The vulnerability is really easy to spot here.  
It checks that len isn't 0 and then copies the report data to the buffer on the stack.  
This buffer can store up to 58 bytes. We can send as many bytes as the MTU allows though.  
This allows writing to the stack and we can easily overwrite the LR stored on the stack, which allows us to jump anywhere in IOS-PAD.  

## Exploiting the bug
At first, I needed a fake controller that can send malicious data to the Wii U.  
I found a repository called [WiimoteEmulator](https://github.com/rnconrad/WiimoteEmulator) on GitHub. Sadly I wasn't able to pair the emulated Wii Remote to my Wii U.  
After hooking into some functions on the IOSU, I figured out the Wii U will try to use something called "Secure Simple Pairing". If the "Secure Simple Pairing" Mode (SSP) is enabled on the client, pairing on the Wii U will fail.  
The fix was relatively simple. All I had to do was to disable SSP on the client while BluuBomb was running.  

We can now send our own data using the WiimoteEmulator.  
All data packets start with `0xa1` if they're sent from the Wii Remote to the Wii U and with `0xa2` if sent from the Wii U to the Wii remote.  
The Wii U will copy all the data behind this byte to the earlier mentioned buffer.  

We can't write to any executable memory without kernel access, so we'll have to use existing instructions stored in executable memory.  
On ARM the function address returned to after calling a function will be stored in the Link Register.  
When a function needs to call another function this Link Register is pushed to the stack and popped back into the program counter when returning from the original function.  
Since we now control the stack we can modify the return addresses stored in it. This allows us to create a so-called ROP chain.  
We can use existing useful instructions, so-called "gadgets", and jump to them.  
When the gadget returns it will read the return address from our controlled stack.  
I've mostly used [ROPgadget](https://github.com/JonathanSalwan/ROPgadget) or Ghidra itself to find useful gadgets.  
We can also execute ARM instructions as Thumb instructions. Thumb is a special mode in ARM processors that allows using 2-byte instructions instead of 4 bytes.  
By loading an address with the last bit set to 1 into the program counter the following instructions will be interpreted as Thumb.  

The stack at the location of the HID buffer looks something like this:
|Stack|
|---- |
|HID buffer |
| r4 - r10 |
| Return address |

We can now create a payload which overwrites our return address.  
This would look something like this:
| Data           | Size | Note
|----------------|------|------
|0xa1            |4     |Indicates that this data is from the Wii Remote
|Can be anything |58    |The data copied to the actual HID buffer
|Other registers |24    |Other registers
|0x11f831f8      |4     |The return address to overwrite (This would jump to IOS_Shutdown and power off the system)


The MTU is around 512 bytes and we only have around 130 bytes until we reached the top of our stack.  
This won't be enough to exploit the kernel or load a kernel binary, so we need a way to load more data into memory.  

I started by making a simple payload which will upload data.  
It uses the 58 bytes of the HID buffer and calls memcpy to copy the data to a location we've specified.  
It then continues with normal execution. To achieve this we only overwrite a specific amount of the stack and jump to a location that would expect the stack pointer at the location it's currently at.  

We can now upload 58 bytes at a time!  
This allows us to upload a bigger ROP chain and a kernel binary.  

Unfortunately, we overwrite the address of the report buffer packet and can no longer free it.  
That means with all the ROP chains we only have 871 bytes for the kernel binary. Ouch, that's small.  

Let's start with the big ROP chain we'll use to gain kernel access.  
We can use a flaw in the IOS_CreateThread call which will clear parts of the specified stack with zeroes. Since this is cleared with kernel permissions we can use this anywhere in memory.  
Zeroes are interpreted as NOPs on ARM. This allows us to patch parts of the `set_panic_behavior` syscall which makes it possible to use it for arbitrary write.  
We can now use this syscall to write bytes anywhere in memory.  
This allows us to write our own instructions and turn the `set_fault_behavior` syscall into a function which copies our kernel binary from memory and executes it with kernel permissions.  
This is highly based on the ROP chain used in Mocha CFW, but adjusted to work in IOS-PAD.  

Now all that was left is a payload that pivots the stack into our bigger ROP chain which is placed into memory.  
We need to pivot the stack or else we'll write over the top of our stack. The IOSU will prevent use from using syscalls if the stack pointer is invalid.  
There only seems to be a single instruction in IOS-PAD which makes it possible to pivot the stack.  
This instruction was `add sp, sp, r2`, which adds the value in register 2 to our stack pointer.  
To properly return from this gadget, without executing instructions we don't want, we use `IOS_CreateThread` again to nop out some instructions.  
We can now offset the stack by any amount we want. The thread we're running on has a stack size of `0x1000`.  
We'll now set register 2 to `-0x600` and run the stack pivot ROP chain.  
This will give us enough space for the big ROP chain.  

And that's basically it. We can now:
- Use the upload payload to upload our kernel binary into unused memory
- Use the upload payload to upload our bigger ROP chain to the stack
- Use the stack pivot payload to pivot the stack into the bigger ROP chain
- Copy the kernel binary into the kernel and run it

The 871 bytes are luckily enough for loading a custom .rpx or a custom fw.img.

## Conclusion

This is the first fully implemented Wii U exploit that directly exploits the IOSU.  
The only thing you need for BluuBomb is a Wii U that is in a state able to pair a Wii Remote and a PC with Bluetooth support.  
For the fw.img loader you also need to be able to access and exit System Settings on the Wii U.  
While the browser is still the more convenient entrypoint, this should be able to repair a few soft bricks.  

Hope this is useful for someone :)

Funnily enough, at the same time as I was finishing implementing this exploit, the Switch got an [update that fixes](https://switchbrew.org/wiki/Switch_System_Flaws#System_Modules) this exact issue.  