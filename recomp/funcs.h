/* Starter declarations for generated C. */
#pragma once
#include "cpu_state.h"

/* Bank 00 */
void I_RESET(CpuState *cpu);
void I_NMI(CpuState *cpu);
void I_IRQ(CpuState *cpu);
void COP_Handler(CpuState *cpu);
void BRK_Handler(CpuState *cpu);
void Abort_Handler(CpuState *cpu);
void MainLoop(CpuState *cpu);
void Init_Hardware(CpuState *cpu);
void VBlank_Wait(CpuState *cpu);

/* Bank 01 */
void City_Update(CpuState *cpu);
void Zone_Update(CpuState *cpu);
void Traffic_Update(CpuState *cpu);
void Power_Update(CpuState *cpu);
void Water_Update(CpuState *cpu);
void Disaster_Update(CpuState *cpu);
void Budget_Calc(CpuState *cpu);
void Tax_Update(CpuState *cpu);
void Population_Update(CpuState *cpu);
void RCI_Update(CpuState *cpu);
void Meteor_Spawn(CpuState *cpu);
void Monster_Spawn(CpuState *cpu);
void Earthquake_Trigger(CpuState *cpu);
void Fire_Update(CpuState *cpu);
void Scenario_Load(CpuState *cpu);
void Scenario_Update(CpuState *cpu);

/* Bank 02 */
void Tile_Draw(CpuState *cpu);
void Tilemap_Render(CpuState *cpu);
void BG_Update(CpuState *cpu);
void Palette_Update(CpuState *cpu);
void Map_Draw(CpuState *cpu);
void Minimap_Draw(CpuState *cpu);
void Iso_Project(CpuState *cpu);
void Iso_Tile_Draw(CpuState *cpu);
void Building_Draw(CpuState *cpu);
void Sprite_Update(CpuState *cpu);

/* Bank 03 */
void Menu_Main(CpuState *cpu);
void Menu_Build(CpuState *cpu);
void Menu_Zone(CpuState *cpu);
void Menu_Query(CpuState *cpu);
void Menu_Budget(CpuState *cpu);
void Menu_Disaster(CpuState *cpu);
void Menu_Scenario(CpuState *cpu);
void Menu_Options(CpuState *cpu);
void Font_Draw(CpuState *cpu);
void Text_Print(CpuState *cpu);
void Number_Draw(CpuState *cpu);
void Panel_Budget(CpuState *cpu);
void Panel_Population(CpuState *cpu);
void Panel_RCI(CpuState *cpu);
void Panel_Map(CpuState *cpu);

/* Bank 04 */
void Input_Read(CpuState *cpu);
void Input_Process(CpuState *cpu);
void Joypad_Read(CpuState *cpu);
void Mouse_Read(CpuState *cpu);
void Cursor_Update(CpuState *cpu);
void Cursor_Draw(CpuState *cpu);
void Tool_Select(CpuState *cpu);
void Tool_Execute(CpuState *cpu);

/* Bank 05 */
void SPC_Upload(CpuState *cpu);
void SPC_Command(CpuState *cpu);
void SPC_Read(CpuState *cpu);
void Music_Play(CpuState *cpu);
void Music_Stop(CpuState *cpu);
void Music_Fade(CpuState *cpu);
void SFX_Play(CpuState *cpu);
void SFX_Update(CpuState *cpu);

/* Bank 06 */
void LC_LZ5_Decompress(CpuState *cpu);
void LC_LZ5_Decode(CpuState *cpu);
void RLE_Decompress(CpuState *cpu);
void Map_Decompress(CpuState *cpu);
void Scenario_Decompress(CpuState *cpu);

/* Bank 07 */
void Title_Init(CpuState *cpu);
void Title_Draw(CpuState *cpu);
void Title_Update(CpuState *cpu);
void Title_Input(CpuState *cpu);
void Logo_Draw(CpuState *cpu);
void Stars_Draw(CpuState *cpu);
void Skyline_Draw(CpuState *cpu);
void Cutscene_Play(CpuState *cpu);