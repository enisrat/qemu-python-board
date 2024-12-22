# Fast Prototyping

This folder is intended to quickly code new device HW and boards (machines). 

- All C source files in this folder (except the skeletons) are automatically compiled

## sysbus_device_skeleton.c

Skeleton template of a simple SysBusDevice that compiles.
It allows for convenient and fast prototyping using Copy&Paste, Find&Replace + *Coding LLMs*
 
### How To
 - Copy this file to a new file inside folder __fastproto__
 - Find&Replace `devxyz` -> your_dev_name
 - Fill in any gaps in the source. You already have code for:
   - __VMSTATE__: all struct members below `_vmstate_saved_offset` are saved automatically
   - 1 __MMIO__ region (adding more is easy by duplicating lines)
   - `NUM_GPIO_IN`, `NUM_GPIO_OUT`: pins in & out
     - Input pin state automtically appears in struct member `in_level`
   - Some qdev __properties__ are predefined. Renaming them simply by Find&Replace
 
### Tipps
 - Keep open a variety of *similar* device source files in your IDE
   - This allows your *Coding LLM* to generate better suggestions by using these as the immediate input context for the Model
