// license:BSD-3-Clause
// copyright-holders:David Haywood,Paul Priest, Luca Elia
/* Jaleco MegaSystem 32 Video Hardware */

/* The Video Hardware is Similar to the Non-MS32 Version of Tetris Plus 2 */

/* Plenty to do, see list in drivers/ms32.c */

/*

priority should be given to
(a) dekluding the priorities, the kludge for kirarast made it easier to emulate the rest of it until then
(b) working out the background registers correctly ...
*/


#include "emu.h"
#include "includes/ms32.h"


// kirarast, tp2m32, and suchie2 require the sprites in a different order

/********** Tilemaps **********/

TILE_GET_INFO_MEMBER(ms32_state::get_ms32_tx_tile_info)
{
	int tileno, colour;

	tileno = m_txram[tile_index *2]   & 0xffff;
	colour = m_txram[tile_index *2+1] & 0x000f;

	tileinfo.set(2,tileno,colour,0);
}

TILE_GET_INFO_MEMBER(ms32_state::get_ms32_roz_tile_info)
{
	int tileno,colour;

	tileno = m_rozram[tile_index *2]   & 0xffff;
	colour = m_rozram[tile_index *2+1] & 0x000f;

	tileinfo.set(0,tileno,colour,0);
}

TILE_GET_INFO_MEMBER(ms32_state::get_ms32_bg_tile_info)
{
	int tileno,colour;

	tileno = m_bgram[tile_index *2]   & 0xffff;
	colour = m_bgram[tile_index *2+1] & 0x000f;

	tileinfo.set(1,tileno,colour,0);
}

TILE_GET_INFO_MEMBER(ms32_state::get_ms32_extra_tile_info)
{
	int tileno,colour;

	tileno = m_f1superb_extraram[tile_index *2]   & 0xffff;
	colour = m_f1superb_extraram[tile_index *2+1] & 0x000f;

	tileinfo.set(3,tileno,colour+0x50,0);
}



void ms32_state::video_start()
{
	m_tx_tilemap     = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(ms32_state::get_ms32_tx_tile_info)),  TILEMAP_SCAN_ROWS,  8, 8,  64, 64);
	m_bg_tilemap     = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(ms32_state::get_ms32_bg_tile_info)),  TILEMAP_SCAN_ROWS, 16,16,  64, 64);
	m_bg_tilemap_alt = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(ms32_state::get_ms32_bg_tile_info)),  TILEMAP_SCAN_ROWS, 16,16, 256, 16); // alt layout, controller by register?
	m_roz_tilemap    = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(ms32_state::get_ms32_roz_tile_info)), TILEMAP_SCAN_ROWS, 16,16, 128,128);

	size_t size = m_sprram.bytes() / 4;
	m_sprram_buffer = make_unique_clear<u16[]>(size);

	/* set up tile layers */
	m_screen->register_screen_bitmap(m_temp_bitmap_tilemaps);
	m_screen->register_screen_bitmap(m_temp_bitmap_sprites);
	m_screen->register_screen_bitmap(m_temp_bitmap_sprites_pri); // not actually being used for rendering, we embed pri info in the raw colour bitmap

	m_temp_bitmap_tilemaps.fill(0);
	m_temp_bitmap_sprites.fill(0);
	m_temp_bitmap_sprites_pri.fill(0);

	m_tx_tilemap->set_transparent_pen(0);
	m_bg_tilemap->set_transparent_pen(0);
	m_bg_tilemap_alt->set_transparent_pen(0);
	m_roz_tilemap->set_transparent_pen(0);

	m_reverse_sprite_order = 1;

	/* i hate per game patches...how should priority really work? tetrisp2.c ? i can't follow it */
	if (!strcmp(machine().system().name,"kirarast"))    m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"tp2m32"))  m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"suchie2"))  m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"suchie2o")) m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"hayaosi3"))    m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"bnstars")) m_reverse_sprite_order = 0;
	if (!strcmp(machine().system().name,"wpksocv2"))    m_reverse_sprite_order = 0;

	// tp2m32 doesn't set the brightness registers so we need sensible defaults
	m_brt[0] = m_brt[1] = 0xffff;

	save_pointer(NAME(m_sprram_buffer), size);
	save_item(NAME(m_irqreq));
	save_item(NAME(m_temp_bitmap_tilemaps));
	save_item(NAME(m_temp_bitmap_sprites));
	save_item(NAME(m_temp_bitmap_sprites_pri));
	save_item(NAME(m_tilemaplayoutcontrol));
	save_item(NAME(m_reverse_sprite_order));
	save_item(NAME(m_flipscreen));
	save_item(NAME(m_brt));
	save_item(NAME(m_brt_r));
	save_item(NAME(m_brt_g));
	save_item(NAME(m_brt_b));
	
	m_dotclock = 0;
	m_crtc.horz_blank = 64;
	m_crtc.horz_display = 320;
	m_crtc.vert_blank = 39;
	m_crtc.vert_display = 224;
	save_item(NAME(m_dotclock));
	save_item(NAME(m_crtc.horz_blank));
	save_item(NAME(m_crtc.horz_display));
	save_item(NAME(m_crtc.vert_blank));
	save_item(NAME(m_crtc.vert_display));
}

VIDEO_START_MEMBER(ms32_state,f1superb)
{
	ms32_state::video_start();

	m_extra_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(ms32_state::get_ms32_extra_tile_info)), TILEMAP_SCAN_ROWS, 2048, 1, 1, 0x400);
}

/********** PALETTE WRITES **********/


void ms32_state::update_color(int color)
{
	int r,g,b;

	/* I'm not sure how the brightness should be applied, currently I'm only
	   affecting bg & sprites, not fg.
	   The second brightness control might apply to shadows, see gametngk.
	 */
	// TODO : p47aces : brightness are disabled sometimes, see https://youtu.be/PQsefFtqAwA
	if (~color & 0x4000)
	{
		r = ((m_palram[color*2] & 0xff00) >>8 ) * m_brt_r / 0x100;
		g = ((m_palram[color*2] & 0x00ff) >>0 ) * m_brt_g / 0x100;
		b = ((m_palram[color*2+1] & 0x00ff) >>0 ) * m_brt_b / 0x100;
	}
	else
	{
		r = ((m_palram[color*2] & 0xff00) >>8 );
		g = ((m_palram[color*2] & 0x00ff) >>0 );
		b = ((m_palram[color*2+1] & 0x00ff) >>0 );
	}

	m_palette->set_pen_color(color,rgb_t(r,g,b));
}

void ms32_state::ms32_brightness_w(offs_t offset, u32 data, u32 mem_mask)
{
	int oldword = m_brt[offset];
	COMBINE_DATA(&m_brt[offset]);

	if (m_brt[offset] != oldword)
	{
		int bank = ((offset & 2) >> 1) * 0x4000;
		//int i;

		if (bank == 0)
		{
			m_brt_r = 0x100 - ((m_brt[0] & 0xff00) >> 8);
			m_brt_g = 0x100 - ((m_brt[0] & 0x00ff) >> 0);
			m_brt_b = 0x100 - ((m_brt[1] & 0x00ff) >> 0);

		//  for (i = 0;i < 0x3000;i++)  // colors 0x3000-0x3fff are not used
		//      update_color(machine(), i);
		}
	}

//popmessage("%04x %04x %04x %04x",m_brt[0],m_brt[1],m_brt[2],m_brt[3]);
}

inline u16 ms32_state::crtc_write_reg(u16 raw_data)
{
	return 0x1000 - (raw_data & 0xfff);
}

inline void ms32_state::crtc_refresh_screen_params()
{
	rectangle visarea = m_screen->visible_area();
	const u16 htotal = m_crtc.horz_blank + m_crtc.horz_display;
	const u16 vtotal = m_crtc.vert_blank + m_crtc.vert_display;
	visarea.set(0, m_crtc.horz_display - 1, 0, m_crtc.vert_display - 1);
	m_screen->configure(htotal, vtotal, visarea, HZ_TO_ATTOSECONDS(m_dotclock & 1 ? 8000000 : 6000000) * htotal * vtotal );
}

// TODO: translate to device
// tetrisp2.cpp: mapped at $ba0000 (Note: ndmnseal sets 0x1000 as vertical blanking then sets it up properly afterwards, safeguard?)
// bnstars1: which presumably sets second screen via a master/slave config?
void ms32_state::crtc_w(offs_t offset, u32 data, u32 mem_mask)
{
	switch(offset)
	{
		case 0x00/4:
			if (ACCESSING_BITS_0_7)
			{
			/* 
			 * ---- x--- used by several games (1->0 in p47aces), unknown
			 * ---- -x-- used by f1superb, unknown
			 * ---- --x- flip screen
			 * ---- ---x dotclock select (1) 8 MHz (0) 6 MHz
			 */
				if ((data & 1) != m_dotclock)
				{
					m_dotclock = data & 0x01;
					crtc_refresh_screen_params();
				}
				m_flipscreen = data & 0x02;
				m_tx_tilemap->set_flip(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
				m_bg_tilemap->set_flip(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
				m_bg_tilemap_alt->set_flip(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);				
			}
			break;
		case 0x04/4: m_crtc.horz_blank = crtc_write_reg(data);	    crtc_refresh_screen_params(); break;
		case 0x08/4: m_crtc.horz_display = crtc_write_reg(data);	crtc_refresh_screen_params(); break;
		case 0x0c/4: logerror("CRTC: HSYNC back porch %d\n", 0x1000 - data);  break;
		case 0x10/4: logerror("CRTC: HSYNC front porch %d\n", 0x1000 - data); break;
		case 0x14/4: m_crtc.vert_blank = crtc_write_reg(data);      crtc_refresh_screen_params(); break;
		case 0x18/4: m_crtc.vert_display = crtc_write_reg(data);    crtc_refresh_screen_params(); break;
		case 0x1c/4: logerror("CRTC: VSYNC back porch %d\n", 0x1000 - data);    break;
		case 0x20/4: logerror("CRTC: VSYNC front porch %d\n", 0x1000 - data);   break;
		default:
			logerror("CRTC: <unknown> register set %04x %04x\n",offset*4, data);
			break;
	}
	
}



/* SPRITES based on tetrisp2 for now, readd priority bits later */
void ms32_state::draw_sprites(bitmap_ind16 &bitmap, bitmap_ind8 &bitmap_pri, const rectangle &cliprect, u16 *sprram_top, size_t sprram_size, int reverseorder)
{
	u16  *source =   sprram_top;
	u16  *finish =   sprram_top + (sprram_size - 0x10) / 2;

	if (reverseorder == 1)
	{
		source  = sprram_top + (sprram_size - 0x10) / 2;
		finish  = sprram_top;
	}

	for (;reverseorder ? (source>=finish) : (source<finish); reverseorder ? (source-=8) : (source+=8))
	{
		bool disable;
		u8 pri;
		bool flipx, flipy;
		u32 code, color;
		u8 tx, ty;
		u16 xsize, ysize;
		s32 sx, sy;
		u16 xzoom, yzoom;

		m_sprite->extract_parameters(true, false, source, disable, pri, flipx, flipy, code, color, tx, ty, xsize, ysize, sx, sy, xzoom, yzoom);

		if (disable || !xzoom || !yzoom)
			continue;

		// passes the priority as the upper bits of the colour
		// for post-processing in mixer instead
		m_sprite->prio_zoom_transpen_raw(bitmap,cliprect,
				code,
				color<<8 | pri<<8,
				flipx, flipy,
				sx, sy,
				tx, ty, xsize, ysize,
				xzoom, yzoom, bitmap_pri, 0, 0);
	}   /* end sprite loop */
}


void ms32_state::draw_roz(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect,int priority)
{
	// TODO: registers 0x40 / 0x44 and 0x50 / 0x54 are used, unknown meaning
    // Given how this works out it is most likely that 0x*0 controls X axis while 0x*4 Y, 
    // nothing is known to diverge between settings so far (i.e. bbbxing sets 0xffff to 0x4* and 0x0000 to 0x5*).
    //             0x4*   0x5*  ROZ should wrap?
    // bbbxing:  0xffff 0x0000  0 (match presentation)
    // gratia:   0x0000 0x0000  1 (sky in stage 2)
    // p47aces:  0xffff 0x0651  0 (title screen)
    // desertwr: 0xffff 0x0651  1 (any stage)
    // f1superb: 0xffff 0x0000  ?
    // suchie2:  0x0000 0x0000  0?
    // bnstars:  0x0000 0x0000  ?
    // hayaosi3: 0x0000 0x0000  ?
    // Maybe wrapping is done by limit boundaries rather than individual bits, so that bbbxing and p47aces abuses of this behaviour?
    // Are we missing a ROZ plane size as well?
    
	if (m_roz_ctrl[0x5c/4] & 1)  /* "super" mode */
	{
		rectangle my_clip;
		int y, maxy;

		my_clip.min_x = cliprect.min_x;
		my_clip.max_x = cliprect.max_x;

		y = cliprect.min_y;
		maxy = cliprect.max_y;

		while (y <= maxy)
		{
			u16 *lineaddr = m_lineram + 8 * (y & 0xff);

			int start2x = (lineaddr[0x00/4] & 0xffff) | ((lineaddr[0x04/4] & 3) << 16);
			int start2y = (lineaddr[0x08/4] & 0xffff) | ((lineaddr[0x0c/4] & 3) << 16);
			int incxx  = (lineaddr[0x10/4] & 0xffff) | ((lineaddr[0x14/4] & 1) << 16);
			int incxy  = (lineaddr[0x18/4] & 0xffff) | ((lineaddr[0x1c/4] & 1) << 16);
			int startx = (m_roz_ctrl[0x00/4] & 0xffff) | ((m_roz_ctrl[0x04/4] & 3) << 16);
			int starty = (m_roz_ctrl[0x08/4] & 0xffff) | ((m_roz_ctrl[0x0c/4] & 3) << 16);
			int offsx  = m_roz_ctrl[0x30/4];
			int offsy  = m_roz_ctrl[0x34/4];

			my_clip.min_y = my_clip.max_y = y;

			offsx += (m_roz_ctrl[0x38/4] & 1) * 0x400;   // ??? gratia, hayaosi1...
			offsy += (m_roz_ctrl[0x3c/4] & 1) * 0x400;   // ??? gratia, hayaosi1...

			/* extend sign */
			if (start2x & 0x20000) start2x |= ~0x3ffff;
			if (start2y & 0x20000) start2y |= ~0x3ffff;
			if (startx & 0x20000) startx |= ~0x3ffff;
			if (starty & 0x20000) starty |= ~0x3ffff;
			if (incxx & 0x10000) incxx |= ~0x1ffff;
			if (incxy & 0x10000) incxy |= ~0x1ffff;

			m_roz_tilemap->draw_roz(screen, bitmap, my_clip,
					(start2x+startx+offsx)<<16, (start2y+starty+offsy)<<16,
					incxx<<8, incxy<<8, 0, 0,
					1, // Wrap
					0, priority);

			y++;
		}
	}
	else    /* "simple" mode */
	{
		int startx = (m_roz_ctrl[0x00/4] & 0xffff) | ((m_roz_ctrl[0x04/4] & 3) << 16);
		int starty = (m_roz_ctrl[0x08/4] & 0xffff) | ((m_roz_ctrl[0x0c/4] & 3) << 16);
		int incxx  = (m_roz_ctrl[0x10/4] & 0xffff) | ((m_roz_ctrl[0x14/4] & 1) << 16);
		int incxy  = (m_roz_ctrl[0x18/4] & 0xffff) | ((m_roz_ctrl[0x1c/4] & 1) << 16);
		int incyy  = (m_roz_ctrl[0x20/4] & 0xffff) | ((m_roz_ctrl[0x24/4] & 1) << 16);
		int incyx  = (m_roz_ctrl[0x28/4] & 0xffff) | ((m_roz_ctrl[0x2c/4] & 1) << 16);
		int offsx  = m_roz_ctrl[0x30/4];
		int offsy  = m_roz_ctrl[0x34/4];

		offsx += (m_roz_ctrl[0x38/4] & 1) * 0x400;   // ??? gratia, hayaosi1...
		offsy += (m_roz_ctrl[0x3c/4] & 1) * 0x400;   // ??? gratia, hayaosi1...

		/* extend sign */
		if (startx & 0x20000) startx |= ~0x3ffff;
		if (starty & 0x20000) starty |= ~0x3ffff;
		if (incxx & 0x10000) incxx |= ~0x1ffff;
		if (incxy & 0x10000) incxy |= ~0x1ffff;
		if (incyy & 0x10000) incyy |= ~0x1ffff;
		if (incyx & 0x10000) incyx |= ~0x1ffff;

		m_roz_tilemap->draw_roz(screen, bitmap, cliprect,
				(startx+offsx)<<16, (starty+offsy)<<16,
				incxx<<8, incxy<<8, incyx<<8, incyy<<8,
				1, // Wrap
				0, priority);
	}
}



u32 ms32_state::screen_update_ms32(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int scrollx,scrolly;
	int asc_pri;
	int scr_pri;
	int rot_pri;

	/* TODO: registers 0x04/4 and 0x10/4 are used too; the most interesting case
	   is gametngk, where they are *usually*, but not always, copies of 0x00/4
	   and 0x0c/4 (used for scrolling).
	   0x10/4 is 0xdf in most games (apart from gametngk's special case), but
	   it's 0x00 in hayaosi1 and kirarast, and 0xe2 (!) in gratia's tx layer.
	   The two registers might be somewhat related to the width and height of the
	   tilemaps, but there's something that just doesn't fit.
	 */
	int i;

	for (i = 0;i < 0x10000;i++) // colors 0x3000-0x3fff are not used
		update_color(i);

	scrollx = m_tx_scroll[0x00/4] + m_tx_scroll[0x08/4] + 0x18;
	scrolly = m_tx_scroll[0x0c/4] + m_tx_scroll[0x14/4];
	m_tx_tilemap->set_scrollx(0, scrollx);
	m_tx_tilemap->set_scrolly(0, scrolly);

	scrollx = m_bg_scroll[0x00/4] + m_bg_scroll[0x08/4] + 0x10;
	scrolly = m_bg_scroll[0x0c/4] + m_bg_scroll[0x14/4];
	m_bg_tilemap->set_scrollx(0, scrollx);
	m_bg_tilemap->set_scrolly(0, scrolly);
	m_bg_tilemap_alt->set_scrollx(0, scrollx);
	m_bg_tilemap_alt->set_scrolly(0, scrolly);


	screen.priority().fill(0, cliprect);

	/* TODO: 0 is correct for gametngk, but break f1superb scrolling grid (text at
	   top and bottom of the screen becomes black on black) */
	m_temp_bitmap_tilemaps.fill(0, cliprect);   /* bg color */

	/* clear our sprite bitmaps */
	m_temp_bitmap_sprites.fill(0, cliprect);
	m_temp_bitmap_sprites_pri.fill(0, cliprect);

	draw_sprites(m_temp_bitmap_sprites, m_temp_bitmap_sprites_pri, cliprect, m_sprram_buffer.get(), 0x20000, m_reverse_sprite_order);


	// TODO: actually understand this (per-scanline priority and alpha-blend over every layer?)
	asc_pri = scr_pri = rot_pri = 0;

	if((m_priram[0x2b00 / 2] & 0x00ff) == 0x0034)
		asc_pri++;
	else
		rot_pri++;

	if((m_priram[0x2e00 / 2] & 0x00ff) == 0x0034)
		asc_pri++;
	else
		scr_pri++;

	// Suchiepai 2 title & Gratia gameplay intermissions uses 0x0f
	// hayaosi3 uses 0x09 during flames screen on attract (text should go above the rest)
	// this is otherwise 0x17 most of the time except for 0x15 in hayaosi3, tetris plus 2 & world pk soccer 2
	// kirarast flips between 0x16 in gameplay and 0x17 otherwise
	if(m_priram[0x3a00 / 2] == 0x09)
		asc_pri = 3;
	if((m_priram[0x3a00 / 2] & 0x0030) == 0x00)
		scr_pri++;
	else
		rot_pri++;

	//popmessage("%02x %02x %02x %d %d %d",m_priram[0x2b00 / 2],m_priram[0x2e00 / 2],m_priram[0x3a00 / 2], asc_pri, scr_pri, rot_pri);

	// tile-tile mixing
	for(int prin=0;prin<4;prin++)
	{
		if(rot_pri == prin)
			draw_roz(screen, m_temp_bitmap_tilemaps, cliprect, 1 << 1);
		else if (scr_pri == prin)
		{
			if (m_tilemaplayoutcontrol&1)
			{
				m_bg_tilemap_alt->draw(screen, m_temp_bitmap_tilemaps, cliprect, 0, 1 << 0);
			}
			else
			{
				m_bg_tilemap->draw(screen, m_temp_bitmap_tilemaps, cliprect, 0, 1 << 0);
			}
		}
		else if(asc_pri == prin)
			m_tx_tilemap->draw(screen, m_temp_bitmap_tilemaps, cliprect, 0, 1 << 2);
	}

	// tile-sprite mixing
	/* this mixing isn't 100% accurate, it should be using ALL the data in
	   the priority ram, probably for per-pixel / pen mixing, or more levels
	   than are supported here..  I don't know, it will need hw tests I think */
	{
		int width = screen.width();
		int height = screen.height();
		pen_t const *const paldata = m_palette->pens();

		bitmap.fill(0, cliprect);

		for (int yy=0;yy<height;yy++)
		{
			u16 const *const srcptr_tile =     &m_temp_bitmap_tilemaps.pix(yy);
			u8 const *const  srcptr_tilepri =  &screen.priority().pix(yy);
			u16 const *const srcptr_spri =     &m_temp_bitmap_sprites.pix(yy);
			//u8 const *const  srcptr_spripri =  &m_temp_bitmap_sprites_pri.pix(yy);
			u32 *const       dstptr_bitmap  =  &bitmap.pix(yy);
			for (int xx=0;xx<width;xx++)
			{
				u16 src_tile  = srcptr_tile[xx];
				u8 src_tilepri = srcptr_tilepri[xx];
				u16 src_spri = srcptr_spri[xx];
				//u8 src_spripri;// = srcptr_spripri[xx];
				u16 spridat = ((src_spri&0x0fff));
				u8  spritepri =     ((src_spri&0xf000) >> 8);
				int primask = 0;

				// get sprite priority value back out of bitmap/colour data (this is done in draw_sprite for standalone hw)
				if (m_priram[(spritepri | 0x0a00 | 0x1500) / 2] & 0x38) primask |= 1 << 0;
				if (m_priram[(spritepri | 0x0a00 | 0x1400) / 2] & 0x38) primask |= 1 << 1;
				if (m_priram[(spritepri | 0x0a00 | 0x1100) / 2] & 0x38) primask |= 1 << 2;
				if (m_priram[(spritepri | 0x0a00 | 0x1000) / 2] & 0x38) primask |= 1 << 3;
				if (m_priram[(spritepri | 0x0a00 | 0x0500) / 2] & 0x38) primask |= 1 << 4;
				if (m_priram[(spritepri | 0x0a00 | 0x0400) / 2] & 0x38) primask |= 1 << 5;
				if (m_priram[(spritepri | 0x0a00 | 0x0100) / 2] & 0x38) primask |= 1 << 6;
				if (m_priram[(spritepri | 0x0a00 | 0x0000) / 2] & 0x38) primask |= 1 << 7;

				// TODO: spaghetti code ...
				if (primask == 0x00)
				{
					if (src_tilepri==0x00)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // best bout boxing title
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x01)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // best bout boxing title
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x02)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // best bout boxing
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x03)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // best bout boxing
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x04)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat];
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x05)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat];
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x06)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat];
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x07)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // desert war radar?
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}


				}
				else if (primask == 0xc0)
				{
					dstptr_bitmap[xx] = paldata[machine().rand()&0xfff];
					popmessage("unhandled priority type %02x, contact MAMEdev",primask);
				}
				else if (primask == 0xcc)
				{
					// hayaosi3 final round ($00 normal, $02 mesh, $03/$05/$07 zoomed in)
					// TODO: may have some blending, hard to say without ref video
					if (src_tilepri & 0x02)
						dstptr_bitmap[xx] = paldata[src_tile];
					else
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat];
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
				}
				else if (primask == 0xf0)
				{
//                  dstptr_bitmap[xx] = paldata[spridat];
					if (src_tilepri==0x00)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // clouds at top gametngk intro
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x01)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // clouds gametngk intro
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x02)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // mode select gametngk
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x03)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // title gametngk
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x04)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // insert coin text on girl gametngk intro
					}
					else if (src_tilepri==0x05)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // insert coin gametngk intro
					}
					else if (src_tilepri==0x06)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // insert coin gametngk intro
					}
					else if (src_tilepri==0x07)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // insert coin gametngk intro
					}
				}
				else if (primask == 0xfc)
				{
					if (src_tilepri==0x00)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // tetrisp intro text
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x01)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // tetrisp intro text
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x02)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // tetrisp story
					}
					else if (src_tilepri==0x03)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // tetrisp fader to game after story
					}
					else if (src_tilepri==0x04)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // credit text tetrisp mode select
					}
					else if (src_tilepri==0x05)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // credit text tetrisp intro
					}
					else if (src_tilepri==0x06)
					{
						//dstptr_bitmap[xx] = paldata[machine().rand()&0xfff];
						dstptr_bitmap[xx] = paldata[src_tile]; // assumed
					}
					else if (src_tilepri==0x07)
					{
						//dstptr_bitmap[xx] = paldata[machine().rand()&0xfff];
						dstptr_bitmap[xx] = paldata[src_tile]; // assumed
					}
				}
				else if (primask == 0xfe)
				{
					if (src_tilepri==0x00)
					{
						if (spridat & 0xff)
							dstptr_bitmap[xx] = paldata[spridat]; // screens in gametngk intro
						else
							dstptr_bitmap[xx] = paldata[src_tile];
					}
					else if (src_tilepri==0x01)
					{
						dstptr_bitmap[xx] = alpha_blend_r32( paldata[src_tile], 0x00000000, 128); // shadow, gametngk title
					}
					else if (src_tilepri==0x02)
					{
						dstptr_bitmap[xx] = alpha_blend_r32( paldata[src_tile], 0x00000000, 128); // shadow, gametngk mode select
					}
					else if (src_tilepri==0x03)
					{
						dstptr_bitmap[xx] = alpha_blend_r32( paldata[src_tile], 0x00000000, 128); // shadow, gametngk title
					}
					else if (src_tilepri==0x04)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // credit text gametngk intro
					}
					else if (src_tilepri==0x05)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // credit text near shadow, gametngk title
					}
					else if (src_tilepri==0x06)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // credit gametngk highscores
					}
					else if (src_tilepri==0x07)
					{
						dstptr_bitmap[xx] = paldata[src_tile]; // assumed
					}
				}
				else if(primask == 0xf8) // gratia ending
				{
					if (spridat & 0xff && src_tilepri == 0x02)
						dstptr_bitmap[xx] = paldata[spridat];
					else
						dstptr_bitmap[xx] = paldata[src_tile];
				}
				else
				{
					// $fa actually used on hayaosi3 second champ transition, unknown purpose
					dstptr_bitmap[xx] = 0;
					popmessage("unhandled priority type %02x, contact MAMEdev",primask);
				}



			}

		}

	}

	return 0;
}

WRITE_LINE_MEMBER(ms32_state::screen_vblank_ms32)
{
	if (state)
	{
		std::copy_n(&m_sprram[0], m_sprram.bytes() / 4, &m_sprram_buffer[0]);
	}
}
