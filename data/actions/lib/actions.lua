SPOTS = {384, 418, 8278, 8592}
ROPABLE = { 294, 369, 370, 383, 392, 408, 409, 427, 428, 430, 462, 469, 470, 482, 484, 485, 489, 924, 3135, 3136, 7933, 7938, 8170, 8286, 8285,
	8284, 8281, 8280, 8279, 8277, 8276, 8323, 8380, 8567, 8585, 8596, 8595, 8249, 8250, 8251, 8252, 8253, 8254, 8255, 8256, 8972, 9606, 9625 }

HOLES = {468, 481, 483, 7932, 8579}
SAND_HOLES = {[9059] = 489, [8568] = 8567}
SAND = 231

JUNGLE_GRASS = {2782, 3985}
SPIDER_WEB = {7538, 7539}
WILD_GROWTH = {1499, 11099}

PUMPKIN = 2683
PUMPKIN_HEAD = 2096

POOL = 2016

SPECIAL_FOODS = {
	[9992] = "Gulp.", [9993] = "Chomp.", [9994] = "Chomp.", [9995] = "Chomp.", [9997] = "Yum.",
	[9998] = "Munch.", [9999] = "Chomp.", [10000] = "Mmmm.", [10001] = "Smack."
}

function destroyItem(cid, itemEx, toPosition)
	if(itemEx.uid <= 65535 or itemEx.actionid > 0) then
		return false
	end

	if not(isInArray({1770, 2098, 1774, 2064, 2094, 2095, 1619, 2602, 3805, 3806}, itemEx.itemid) or
		(itemEx.itemid >= 1724 and itemEx.itemid <= 1741) or
		(itemEx.itemid >= 2581 and itemEx.itemid <= 2588) or
		(itemEx.itemid >= 1747 and itemEx.itemid <= 1753) or
		(itemEx.itemid >= 1714 and itemEx.itemid <= 1717) or
		(itemEx.itemid >= 1650 and itemEx.itemid <= 1653) or
		(itemEx.itemid >= 1666 and itemEx.itemid <= 1677) or
		(itemEx.itemid >= 1614 and itemEx.itemid <= 1616) or
		(itemEx.itemid >= 3813 and itemEx.itemid <= 3820) or
		(itemEx.itemid >= 3807 and itemEx.itemid <= 3810) or
		(itemEx.itemid >= 2080 and itemEx.itemid <= 2085) or
		(itemEx.itemid >= 2116 and itemEx.itemid <= 2119)) then
		return false
	end

	if(math.random(1, 7) == 1) then
		if(isInArray({1738, 1739, 1770, 2098, 1774, 1775, 2064}, itemEx.itemid) or
			(itemEx.itemid >= 2581 and itemEx.itemid <= 2588)) then
				doCreateItem(2250, 1, toPosition)
		elseif((itemEx.itemid >= 1747 and itemEx.itemid <= 1749) or itemEx.itemid == 1740) then
			doCreateItem(2251, 1, toPosition)
		elseif((itemEx.itemid >= 1714 and itemEx.itemid <= 1717)) then
			doCreateItem(2252, 1, toPosition)
		elseif((itemEx.itemid >= 1650 and itemEx.itemid <= 1653) or
			(itemEx.itemid >= 1666 and itemEx.itemid <= 1677) or
			(itemEx.itemid >= 1614 and itemEx.itemid <= 1616) or
			(itemEx.itemid >= 3813 and itemEx.itemid <= 3820) or
			(itemEx.itemid >= 3807 and itemEx.itemid <= 3810)) then
				doCreateItem(2253, 1, toPosition)
		elseif((itemEx.itemid >= 1724 and itemEx.itemid <= 1737) or
			(itemEx.itemid >= 2080 and itemEx.itemid <= 2085) or
			(itemEx.itemid >= 2116 and itemEx.itemid <= 2119) or
			isInArray({2094, 2095}, itemEx.itemid)) then
				doCreateItem(2254, 1, toPosition)
		elseif((itemEx.itemid >= 1750 and itemEx.itemid <= 1753) or isInArray({1619, 1741}, itemEx.itemid)) then
			doCreateItem(2255, 1, toPosition)
		elseif(itemEx.itemid == 2602) then
			doCreateItem(2257, 1, toPosition)
		elseif(itemEx.itemid == 3805 or itemEx.itemid == 3806) then
			doCreateItem(2259, 1, toPosition)
		end

		doRemoveItem(itemEx.uid, 1)
	end

	doSendMagicEffect(toPosition, CONST_ME_POFF)
	return true
end

TOOLS = {}
TOOLS.ROPE = function(cid, item, fromPosition, itemEx, toPosition)
	if(toPosition.x == CONTAINER_POSITION) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	toPosition.stackpos = STACKPOS_GROUND
	local ground = getThingFromPos(toPosition)
	if(isInArray(SPOTS, ground.itemid)) then
		doTeleportThing(cid, {x = toPosition.x, y = toPosition.y + 1, z = toPosition.z - 1}, false)
		return true
	elseif(isInArray(ROPABLE, itemEx.itemid)) then
		local hole = getThingFromPos({x = toPosition.x, y = toPosition.y, z = toPosition.z + 1, stackpos = STACKPOS_TOP_MOVEABLE_ITEM_OR_CREATURE})
		if(isPlayer(hole.uid) and (not isPlayerGhost(hole.uid) or getPlayerGhostAccess(cid) >= getPlayerGhostAccess(hole.uid))) then
			doTeleportThing(hole.uid, {x = toPosition.x, y = toPosition.y + 1, z = toPosition.z}, false)
		else
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		end

		return true
	end

	return false
end

TOOLS.PICK = function(cid, item, fromPosition, itemEx, toPosition)
	local ground = getThingFromPos({x = toPosition.x, y = toPosition.y, z = toPosition.z + 1, stackpos = STACKPOS_GROUND})
	if(isInArray(SPOTS, ground.itemid) and isInArray({354, 355}, itemEx.itemid)) then
		doTransformItem(itemEx.uid, 392)
		doDecayItem(itemEx.uid)

		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return true
	end

	if(itemEx.itemid == 7200) then
		doTransformItem(itemEx.uid, 7236)
		doSendMagicEffect(toPosition, CONST_ME_BLOCKHIT)
		return true
	end

	return false
end

TOOLS.MACHETE = function(cid, item, fromPosition, itemEx, toPosition, destroy)
	if(isInArray(JUNGLE_GRASS, itemEx.itemid)) then
		doTransformItem(itemEx.uid, itemEx.itemid - 1)
		doDecayItem(itemEx.uid)
		return true
	end

	if(isInArray(SPIDER_WEB, itemEx.itemid)) then
		doTransformItem(itemEx.uid, (itemEx.itemid + 6))
		doDecayItem(itemEx.uid)
		return true
	end

	if(isInArray(WILD_GROWTH, itemEx.itemid)) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doRemoveItem(itemEx.uid)
		return true
	end

	return destroy and destroyItem(cid, itemEx, toPosition) or false
end

TOOLS.SHOVEL = function(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(HOLES, itemEx.itemid)) then
		if(itemEx.itemid ~= 8579) then
			itemEx.itemid = itemEx.itemid + 1
		else
			itemEx.itemid = 8585
		end

		doTransformItem(itemEx.uid, itemEx.itemid)
		doDecayItem(itemEx.uid)
		return true
	elseif(SAND_HOLES[itemEx.itemid] ~= nil) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doTransformItem(itemEx.uid, SAND_HOLES[itemEx.itemid])

		doDecayItem(itemEx.uid)
		return true
	elseif(itemEx.itemid == SAND and not isRookie(cid)) then
		local rand = math.random(1, 100)
		if(rand >= 1 and rand <= 5) then
			doCreateItem(ITEM_SCARAB_COIN, 1, toPosition)
		elseif(rand > 85) then
			doCreateMonster("Scarab", toPosition, false)
		end

		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return true
	end

	return false
end

TOOLS.SCYTHE = function(cid, item, fromPosition, itemEx, toPosition, destroy)
	if(itemEx.itemid == 2739) then
		doTransformItem(itemEx.uid, 2737)
		doCreateItem(2694, 1, toPosition)

		doDecayItem(itemEx.uid)
		return true
	end

	return destroy and destroyItem(cid, itemEx, toPosition) or false
end

TOOLS.KNIFE = function(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.itemid ~= PUMPKIN) then
		return false
	end

	doTransformItem(itemEx.uid, PUMPKIN_HEAD)
	return true
end