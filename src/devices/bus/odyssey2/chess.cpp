// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/******************************************************************************

Videopac C7010 Chess Module emulation
The chess engine is "Gambiet", written by Wim Rens

Hardware notes:
- NSC800 (Z80-compatible) @ 4.43MHz
- 8KB ROM, 2KB RAM

Service manual with schematics is available.

TODO:
- this code is just a stub... hence, almost everything is still to do!
  most importantly, missing 8KB ROM dump

******************************************************************************/

#include "emu.h"
#include "chess.h"


//-------------------------------------------------
//  o2_chess_device - constructor
//-------------------------------------------------

DEFINE_DEVICE_TYPE(O2_ROM_CHESS, o2_chess_device, "o2_chess", "Odyssey 2 Videopac Chess Module")


o2_chess_device::o2_chess_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: o2_rom_device(mconfig, O2_ROM_CHESS, tag, owner, clock)
	, m_cpu(*this, "subcpu")
{
}


void o2_chess_device::chess_mem(address_map &map)
{
	map(0x0000, 0x07ff).r(FUNC(o2_chess_device::read_rom04));
}

void o2_chess_device::chess_io(address_map &map)
{
	map.unmap_value_high();
	map.global_mask(0xff);
}


//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void o2_chess_device::device_add_mconfig(machine_config &config)
{
	NSC800(config, m_cpu, 4.433619_MHz_XTAL);
	m_cpu->set_addrmap(AS_PROGRAM, &o2_chess_device::chess_mem);
	m_cpu->set_addrmap(AS_IO, &o2_chess_device::chess_io);
}
