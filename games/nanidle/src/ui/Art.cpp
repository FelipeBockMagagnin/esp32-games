#include "Art.h"

static const SpriteDef *const SPRITES[(uint8_t)Art::COUNT] = {
    &sprite_dull_branch, &sprite_cool_branch, &sprite_great_branch, &sprite_slime,
    &sprite_frog,        &sprite_sad_frog,    &sprite_frog_wizard,  &sprite_small_rat,
    &sprite_big_bad_rat, &sprite_gamer_mouse, &sprite_spider,       &sprite_gnome,
    &sprite_bard_gnome,  &sprite_goblin,      &sprite_zombie,       &sprite_gnome_band,
};

static const SpriteDef *const BACKDROPS[(uint8_t)Backdrop::COUNT] = {
    &background_tutorial,
    &background_sewers,
    &background_forest,
};

static const uint16_t *const ICONS[(uint8_t)ItemArt::COUNT] = {
    image_item_leaf_hat_pixels,   image_item_leather_cap_pixels, image_item_fig_leaf_pixels,
    image_item_loincloth_pixels,  image_item_dull_stick_pixels,  image_item_good_stick_pixels,
    image_item_great_stick_pixels, image_item_wilson_pixels,     image_icon_boots_01a_pixels,
    image_icon_boots_01c_pixels,  image_icon_armor_01a_pixels,   image_icon_armor_01c_pixels,
    image_icon_ring_01a_pixels,   image_icon_necklace_01b_pixels, image_icon_gloves_01a_pixels,
    image_icon_sword_01a_pixels,
};

const SpriteDef *spriteFor(Art art)
{
    return SPRITES[(uint8_t)art];
}

const SpriteDef *backdropFor(Backdrop backdrop)
{
    return BACKDROPS[(uint8_t)backdrop];
}

const uint16_t *iconFor(ItemArt art)
{
    return ICONS[(uint8_t)art];
}
