local KIT_GREEN = 7904
local KIT_RED = 7905
local KIT_YELLOW = 7906
local KIT_REMOVAL = 7907

local BEDS = {
	[KIT_GREEN] =	{{7811, 7812}, {7813, 7814}},
	[KIT_RED] =	{{7815, 7816}, {7817, 7818}},
	[KIT_YELLOW] =	{{7819, 7820}, {7821, 7822}},
	[KIT_REMOVAL] =	{{1754, 1755}, {1760, 1761}}
}

local function internalBedTransform(item, itemEx, toPosition, ids)
	doTransformItem(itemEx.uid, ids[1])
	doTransformItem(getThingfromPos(toPosition).uid, ids[2])

	doSendMagicEffect(getThingPos(itemEx.uid), CONST_ME_POFF)
	doSendMagicEffect(toPosition, CONST_ME_POFF)

	doRemoveItem(item.uid)
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local newBed = BEDS[item.itemid]
	if not newBed or getTileHouseInfo(getCreaturePosition(cid)) == FALSE then
		return FALSE
	end

	--TODO
	--Is it possible in real tibia, to use same modification on current used?
	if isInArray({newBed[1][1], newBed[2][1]}, itemEx.itemid) == TRUE then
		doPlayerSendCancel(cid, "You already have this bed modification.")
		return TRUE
	end

	for kit, bed in pairs(BEDS) do
		if bed[1][1] == itemEx.itemid or itemEx.itemid == 1758 then
			toPosition.y = toPosition.y + 1
			internalBedTransform(item, itemEx, toPosition, newBed[1])
			break
		elseif bed[2][1] == itemEx.itemid or itemEx.itemid == 1756 then
			toPosition.x = toPosition.x + 1
			internalBedTransform(item, itemEx, toPosition, newBed[2])
			break
		end
	end

	return TRUE
end
