-- Include the Advanced NPC System
dofile('data/npc/lib/npcsystem/npcsystem.lua')

function doMessageCheck(message, keyword)
	if(type(keyword) == "table") then
		return table.isStrIn(keyword, message)
	else	
		local a, b = message:lower():find(keyword:lower())
		if(a ~= nil and b ~= nil) then
			return true
		end
	end

	return false
end

function selfGotoIdle()
	following = false
	attacking = false
	selfAttackCreature(0)
	target = 0
end

function selfMoveToCreature(id)
	if(id == 0 or id == nil) then
		selfGotoIdle()
	end

	local tx, ty, tz = getCreaturePosition(id)
	if(tx == nil) then
		selfGotoIdle()
	else
		moveToPosition(tx, ty, tz)
	end
end

function getDistanceToCreature(id)
	if(id == 0 or id == nil) then
		selfGotoIdle()
	end

	local cx, cy, cz = getCreaturePosition(id)
	if(cx == nil) then
		return nil
	end

	local sx, sy, sz = selfGetPosition()
	return math.max(math.abs(sx - cx), math.abs(sy - cy))
end

function doNpcSellItem(cid, itemid, amount, subType, ignoreCap, inBackpacks, backpack)
	local amount = amount or 1
	local subType = subType or 0
	local ignoreCap = ignoreCap and TRUE or FALSE

	local item = 0
	if(isItemStackable(itemid) == TRUE) then
		item = doCreateItemEx(itemid, amount)
		if(doPlayerAddItemEx(cid, item, ignoreCap) ~= RETURNVALUE_NOERROR) then
			return 0, 0
		end

		return amount, 0
	end

	local a = 0
	if(inBackpacks) then
		local container = doCreateItemEx(backpack, 1)
		local b = 1
		for i = 1, amount do
			item = doAddContainerItem(container, itemid, subType)
			if(itemid == ITEM_PARCEL) then
				doAddContainerItem(item, ITEM_LABEL)
			end

			if(isInArray({(getContainerCapById(backpack) * b), amount}, i) == TRUE) then
				if(doPlayerAddItemEx(cid, container, ignoreCap) ~= RETURNVALUE_NOERROR) then
					break
				end

				a = i
				if(amount > i) then
					container = doCreateItemEx(backpack, 1)
					b = b + 1
				end
			end
		end
		return a, b
	end

	for i = 1, amount do
		item = doCreateItemEx(itemid, subType)
		if(itemid == ITEM_PARCEL) then
			doAddContainerItem(item, ITEM_LABEL)
		end

		if(doPlayerAddItemEx(cid, item, ignoreCap) ~= RETURNVALUE_NOERROR) then
			break
		end
		a = i
	end
	return a, 0
end

function doPosRemoveItem(_itemid, n, position)
	local thing = getThingFromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if(thing.itemid == _itemid) then
		doRemoveItem(thing.uid, n)
		return true
	end

	return false
end

function isPlayerPremiumCallback(cid)
	return isPremium(cid) == TRUE
end

function selfSayChannel(cid, message)
	return selfSay(message, cid, FALSE)
end

msgcontains = doMessageCheck
moveToPosition = selfMoveTo
moveToCreature = selfMoveToCreature
selfMoveToPosition = selfMoveTo
doNpcBuyItem = doPlayerRemoveItem
