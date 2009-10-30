local config = {
	level = 2
}

local SKINS = {
	-- Minotaurs
	[2830] = {25000, 5878},
	[2871] = {25000, 5878},
	[2866] = {25000, 5878},
	[2876] = {25000, 5878},
	[3090] = {25000, 5878},

	-- Lizards
	[4259] = {25000, 5876},
	[4262] = {25000, 5876},
	[4256] = {25000, 5876},

	-- Dragons
	[3104] = {25000, 5877},
	[2844] = {25000, 5877},

	-- Dragon Lords
	[2881] = {25000, 5948},

	-- Behemoths
	[2931] = {25000, 5893},

	-- Bone Beasts
	[3031] = {25000, 5925}
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(getPlayerLevel(cid) < config.level) then
		doPlayerSendCancel(cid, "You have to be at least Level " .. config.level .. " to use this tool.")
		return true
	end

	local skin = SKINS[itemEx.itemid]
	if(not skin) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	local random, effect = math.random(1, 100000), CONST_ME_GROUNDSHAKER
	if(random <= skin[1]) then
		doPlayerAddItem(cid, skin[2], 1)
	elseif(skin[3] and random >= skin[3]) then
		doPlayerAddItem(cid, skin[4], 1)
	else
		effect = CONST_ME_POFF
	end

	doSendMagicEffect(toPosition, effect)
	doTransformItem(itemEx.uid, itemEx.itemid + 1)
	return true
end
