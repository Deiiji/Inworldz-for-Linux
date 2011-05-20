/** 
 * @file llpanelgeneral.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

//file include
#include "llpanelgeneral.h"

// project includes
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lluictrlfactory.h"
#include "llurlsimstring.h"
#include "llviewercontrol.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llpanellogin.h"
#include "llviewerregion.h"

LLPanelGeneral::LLPanelGeneral()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_general.xml");
}

BOOL LLPanelGeneral::postBuild()
{
	LLComboBox* fade_out_combobox = getChild<LLComboBox>("fade_out_combobox");
	fade_out_combobox->setCurrentByIndex(gSavedSettings.getS32("RenderName"));

	LLComboBox* combo = getChild<LLComboBox>("default_location_combo");
	childSetCommitCallback("default_location_combo", onLocationChanged, this);
	combo->setAllowTextEntry(TRUE, 128, FALSE);

	// The XML file loads the combo with the following labels:
	// 0 - "My Home"
	// 1 - "My Last Location"
	// 2 - "<Type region name>"

	BOOL login_last = gSavedSettings.getBOOL("LoginLastLocation");
	std::string sim_string = LLURLSimString::sInstance.mSimString;
	if (!sim_string.empty())
	{
		// Replace "<Type region name>" with this region name
		combo->remove(2);
		combo->add( sim_string );
		combo->setTextEntry(sim_string);
		combo->setCurrentByIndex( 2 );
	}
	else if (login_last)
	{
		combo->setCurrentByIndex( 1 );
	}
	else
	{
		combo->setCurrentByIndex( 0 );
	}

	childSetValue("hide_location_checkbox", gSavedSettings.getBOOL("HideStartLocation"));
	childSetValue("show_all_title_checkbox", gSavedSettings.getBOOL("RenderHideGroupTitleAll"));
	childSetValue("language_is_public", gSavedSettings.getBOOL("LanguageIsPublic"));

	childSetValue("show_my_name_checkbox", gSavedSettings.getBOOL("RenderNameHideSelf"));
	childSetValue("small_avatar_names_checkbox", gSavedSettings.getBOOL("SmallAvatarNames"));
	childSetValue("show_my_title_checkbox", gSavedSettings.getBOOL("RenderHideGroupTitle"));
	childSetValue("afk_timeout_spinner", gSavedSettings.getF32("AFKTimeout"));
	childSetValue("afk_timeout_checkbox", gSavedSettings.getBOOL("AllowIdleAFK"));
	childSetValue("notify_money_change_checkbox", gSavedSettings.getBOOL("NotifyMoneyChange"));

	getChild<LLColorSwatchCtrl>("effect_color_swatch")->set(gSavedSettings.getColor4("EffectColor"));

	childSetValue("ui_scale_slider", gSavedSettings.getF32("UIScaleFactor"));
	childSetValue("ui_auto_scale", gSavedSettings.getBOOL("UIAutoScale"));

	LLComboBox* crash_behavior_combobox = getChild<LLComboBox>("crash_behavior_combobox");
	crash_behavior_combobox->setCurrentByIndex(gCrashSettings.getS32(CRASH_BEHAVIOR_SETTING));
	
	childSetValue("language_combobox", 	gSavedSettings.getString("Language"));

	childSetAction("reset_ui_size", onClickResetUISize, this);
	
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose = gAgent.getID().notNull() &&
					 (gAgent.isMature() || gAgent.isGodlike());
	
	if (can_choose)
	{

		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlike())
		{
			LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
			// we're going to remove the adult entry from the combo. This obviously depends
			// on the order of items in the XML file, but there doesn't seem to be a reasonable
			// way to depend on the field in XML called 'name'.
			maturity_combo->remove(0);
		}
	}
	
	U32 preferred_maturity = gSavedSettings.getU32("PreferredMaturity");
	childSetValue("maturity_desired_combobox", int(preferred_maturity));
	std::string selected_item_label = getChild<LLComboBox>("maturity_desired_combobox")->getSelectedItemLabel();
	childSetValue("maturity_desired_textbox", selected_item_label);
	
	childSetVisible("maturity_desired_combobox", can_choose);
	childSetVisible("maturity_desired_textbox",	!can_choose);
			
	return TRUE;
}

LLPanelGeneral::~LLPanelGeneral()
{
	// Children all cleaned up by default view destructor.
}

void LLPanelGeneral::apply()
{
	LLComboBox* fade_out_combobox = getChild<LLComboBox>("fade_out_combobox");
	gSavedSettings.setS32("RenderName", fade_out_combobox->getCurrentIndex());
	
	LLComboBox* loc_combo = getChild<LLComboBox>("default_location_combo");
	gSavedSettings.setBOOL("LoginLastLocation", loc_combo->getCurrentIndex() == 1);
	if (!loc_combo->getValue().asString().empty() && 
		loc_combo->getSelectedItemLabel() != "<Type region name>")
	{
		LLURLSimString::setString(loc_combo->getValue().asString());
	}
	LLPanelLogin::refreshLocation(false);

	gSavedSettings.setBOOL("HideStartLocation", childGetValue("hide_location_checkbox"));
	gSavedSettings.setBOOL("RenderHideGroupTitleAll", childGetValue("show_all_title_checkbox"));
	gSavedSettings.setBOOL("LanguageIsPublic", childGetValue("language_is_public"));
	gSavedSettings.setBOOL("RenderNameHideSelf", childGetValue("show_my_name_checkbox"));
	gSavedSettings.setBOOL("SmallAvatarNames", childGetValue("small_avatar_names_checkbox"));
	gSavedSettings.setBOOL("RenderHideGroupTitle", childGetValue("show_my_title_checkbox"));
	gSavedSettings.setF32("AFKTimeout", childGetValue("afk_timeout_spinner").asReal());
	gSavedSettings.setBOOL("AllowIdleAFK", childGetValue("afk_timeout_checkbox"));
	gSavedSettings.setBOOL("NotifyMoneyChange", childGetValue("notify_money_change_checkbox"));
	gSavedSettings.setColor4("EffectColor", childGetValue("effect_color_swatch"));
	gSavedSettings.setF32("UIScaleFactor", childGetValue("ui_scale_slider").asReal());
	gSavedSettings.setBOOL("UIAutoScale", childGetValue("ui_auto_scale"));
	gSavedSettings.setString("Language", childGetValue("language_combobox"));


	LLComboBox* crash_behavior_combobox = getChild<LLComboBox>("crash_behavior_combobox");
	gCrashSettings.setS32(CRASH_BEHAVIOR_SETTING, crash_behavior_combobox->getCurrentIndex());
	
	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose = gAgent.getID().notNull() &&
					(gAgent.isMature() || gAgent.isGodlike());
	
	if (can_choose)
	{
		int preferred_maturity = childGetValue("maturity_desired_combobox").asInteger();
		
		if (preferred_maturity != gSavedSettings.getU32("PreferredMaturity"))
		{
			gSavedSettings.setU32("PreferredMaturity", preferred_maturity);
			gAgent.sendMaturityPreferenceToServer(preferred_maturity);
		}
	}

	// Keep gAllowIdleAFK around for performance reasons -- MC
	if (gAllowIdleAFK != (BOOL)childGetValue("afk_timeout_checkbox"))
	{
		gAllowIdleAFK = childGetValue("afk_timeout_checkbox");
	}
}

void LLPanelGeneral::cancel()
{
}

// static
void LLPanelGeneral::onClickResetUISize(void* user_data)
{
	LLPanelGeneral* self = (LLPanelGeneral*)user_data;
	F32 def = gSavedSettings.getControl("UIScaleFactor")->getDefault().asReal();
	self->childSetValue("ui_scale_slider", def);
	gSavedSettings.setBOOL("UIAutoScale", FALSE);
}

// static
void LLPanelGeneral::onLocationChanged(LLUICtrl* ctrl, void* data)
{
	LLPanelGeneral* self = (LLPanelGeneral*)data;
	if (self->getChild<LLComboBox>("default_location_combo")->getCurrentIndex() == 2)
	{
		self->getChild<LLComboBox>("default_location_combo")->setTextEntry(LLURLSimString::sInstance.mSimString);
	}
}
