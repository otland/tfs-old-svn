local outfits = {
	[0] = {
		["Citizen"] = 136,
		["Hunter"] = 137,
		["Mage"] = 138,
		["Knight"] = 139,
		["Noblewoman"] = 140,
		["Summoner"] = 141,
		["Warrior"] = 142,
		["Barbarian"] = 147,
		["Druid"] = 148,
		["Wizard"] = 149,
		["Oriental"] = 150,
		["Pirate"] = 155,
		["Assassin"] = 156,
		["Beggar"] = 157,
		["Shaman"] = 158,
		["Norsewoman"] = 252,
		["Nightmare"] = 269,
		["Jester"] = 270,
		["Brotherhood"] = 279,
		["Demonhunter"] = 288,
		["Yalaharian"] = 324,
		["Wedding"] = 329
	},
	[1] = {
		["Citizen"] = 128,
		["Hunter"] = 129,
		["Mage"] = 130,
		["Knight"] = 131,
		["Nobleman"] = 132,
		["Summoner"] = 133,
		["Warrior"] = 134,
		["Barbarian"] = 143,
		["Druid"] = 144,
		["Wizard"] = 145,
		["Oriental"] = 146,
		["Pirate"] = 151,
		["Assassin"] = 152,
		["Beggar"] = 153,
		["Shaman"] = 154,
		["Norseman"] = 251,
		["Nightmare"] = 268,
		["Jester"] = 273,
		["Brotherhood"] = 278,
		["Demonhunter"] = 289,
		["Yalaharian"] = 325,
		["Wedding"] = 328
	}
}
function onSay(cid, words, param)
	local t = param:explode(",")
	local outfit = outfits[getPlayerSex(cid)]
	if not t[1] then
		doPlayerSendTextMessage(cid, 27, "Command requires param.")
		return TRUE
	end
	if isNumber(t[2]) == FALSE then
		doPlayerSendTextMessage(cid, 27, "Command requires numeric param.")
		return TRUE
	end
	if canPlayerWearOutfit(cid, outfits[t[1]], t[2]) then
		doPlayerSendTextMessage(cid, 27, "You already have this addon.")
		return TRUE
	end
	doPlayerAddOutfit(cid, outfits[t[1]], t[2])
	return TRUE
end