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
	[2931] = {25000, 5930, 90000, 5893},

	-- Bone Beasts
	[3031] = {25000, 5925}
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(getPlayerLevel(cid) <= 1) then
		doPlayerSendCancel(cid, "You have to be at least Level 2 to use this tool.")
		return TRUE
	end

	local skin = SKINS[itemEx.itemid]
	if(skin == nil) then
		doPlayerSendCancel(cid, "Sorry, not possible.")
		return TRUE
	end

	local random = math.random(1, 100000)
	if(random <= skin[1]) then
		doSendMagicEffect(toPosition, CONST_ME_GROUNDSHAKER)
		doPlayerAddItem(cid, skin[2], 1)
	elseif(skin[3] and random >= skin[3]) then
		doSendMagicEffect(toPosition, CONST_ME_GROUNDSHAKER)
		doPlayerAddItem(cid, skin[4], 1)
	else
		doSendMagicEffect(toPosition, CONST_ME_POFF)
	end

	doTransformItem(itemEx.uid, itemEx.itemid + 1)
	return TRUE
end
