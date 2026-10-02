#ifndef ASSETS_H
#define ASSETS_H

#include <Arduino.h>

// 3x5 small arrow pointing left
extern const unsigned char image_ButtonLeftSmall_bits[];

// 9x9 Ok button
extern const unsigned char image_Ok_btn_bits[];

extern const uint16_t image_armor_01b_pixels[];
extern const unsigned char image_ButtonRightSmall_bits[];
extern const unsigned char image_cards_hearts_bits[];
extern const uint16_t image_rato_pixels[];
extern const uint16_t image_sword_02b_pixels[];

extern const unsigned char image_display_brightness_bits[];
extern const uint16_t image_Icon31_33_pixels[];
extern const unsigned char image_weather_humidity_white_bits[];
extern const unsigned char image_weather_temperature_bits[];


extern const unsigned char image_cursor_black_white_bits[];
extern const unsigned char image_device_key_retro_bits[];
extern const unsigned char image_door_closed_bits[];

// 16x16 building icons, one per BUILDINGS row
extern const uint16_t image_building_pickaxe_pixels[];
extern const uint16_t image_building_minecart_pixels[];
extern const uint16_t image_building_drill_pixels[];
extern const uint16_t image_building_excavator_pixels[];
extern const uint16_t image_building_ore_mill_pixels[];
extern const uint16_t image_building_smelter_pixels[];
extern const uint16_t image_building_deep_shaft_pixels[];
extern const uint16_t image_building_rune_forge_pixels[];
extern const uint16_t image_building_ore_barge_pixels[];
extern const uint16_t image_building_transmuter_pixels[];

// 16x16 upgrade category icons
extern const uint16_t image_upgrade_production_pixels[];
extern const uint16_t image_upgrade_click_pixels[];
extern const uint16_t image_upgrade_heart_pixels[];
extern const uint16_t image_upgrade_map_pixels[];

// 16x16 award icons
extern const uint16_t image_award_trophy_pixels[];
extern const uint16_t image_award_skull_pixels[];
extern const uint16_t image_award_scroll_pixels[];

// 16x16 item icons, one per ITEMS row (Iron Mail and Iron Sword use the armor and sword above)
extern const uint16_t image_item_cloth_hood_pixels[];
extern const uint16_t image_item_iron_helm_pixels[];
extern const uint16_t image_item_steel_helm_pixels[];
extern const uint16_t image_item_rune_helm_pixels[];
extern const uint16_t image_item_cloth_vest_pixels[];
extern const uint16_t image_item_steel_plate_pixels[];
extern const uint16_t image_item_rune_plate_pixels[];
extern const uint16_t image_item_cloth_pants_pixels[];
extern const uint16_t image_item_iron_legs_pixels[];
extern const uint16_t image_item_steel_legs_pixels[];
extern const uint16_t image_item_rune_legs_pixels[];
extern const uint16_t image_item_worn_boots_pixels[];
extern const uint16_t image_item_iron_boots_pixels[];
extern const uint16_t image_item_steel_boots_pixels[];
extern const uint16_t image_item_rune_boots_pixels[];
extern const uint16_t image_item_rag_gloves_pixels[];
extern const uint16_t image_item_iron_gloves_pixels[];
extern const uint16_t image_item_steel_grips_pixels[];
extern const uint16_t image_item_rune_grips_pixels[];
extern const uint16_t image_item_rusty_sword_pixels[];
extern const uint16_t image_item_steel_sword_pixels[];
extern const uint16_t image_item_rune_blade_pixels[];
extern const uint16_t image_item_wood_shield_pixels[];
extern const uint16_t image_item_iron_shield_pixels[];
extern const uint16_t image_item_steel_guard_pixels[];
extern const uint16_t image_item_rune_aegis_pixels[];
extern const uint16_t image_item_tin_ring_pixels[];
extern const uint16_t image_item_gold_ring_pixels[];
extern const uint16_t image_item_ruby_ring_pixels[];
extern const uint16_t image_item_miner_ring_pixels[];
extern const uint16_t image_item_bone_charm_pixels[];
extern const uint16_t image_item_jade_amulet_pixels[];
extern const uint16_t image_item_gold_amulet_pixels[];
extern const uint16_t image_item_core_sigil_pixels[];

#endif // ASSETS_H
