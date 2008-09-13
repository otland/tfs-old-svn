-- Including the Advanced NPC System
dofile('data/npc/lib/npcsystem/npcsystem.lua')

do
	doPlayerAddStackable = doPlayerAddItem
	--Returns table with UIDs of added items
	doPlayerAddItem = function(cid, itemid, amount, subType)
		local amount = amount or 1
		local subAmount = 0
		local subType = subType or 0

		if(isItemStackable(itemid) == TRUE) then
			return doPlayerAddStackable(cid, itemid, amount), amount
		end

		local items = {}
		local ret = 0
		local a = 0
		for i = 1, amount do
			items[i] = doCreateItemEx(itemid, subType)
			ret = doPlayerAddItemEx(cid, items[i], 1)
			if(ret ~= RETURNVALUE_NOERROR) then
				break
			end
			a = a + 1
		end

		return items, a
	end
end

function getDistanceToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end

	local creaturePosition = getCreaturePosition(id)
	cx = creaturePosition.x
	cy = creaturePosition.y
	cz = creaturePosition.z
	if cx == nil then
		return nil
	end

	sx, sy, sz = selfGetPosition()
	return math.max(math.abs(sx - cx), math.abs(sy - cy))
end

function moveToPosition(x,y,z)
	selfMoveTo(x, y, z)
end

function moveToCreature(id)
	if id == 0 or id == nil then
		selfGotoIdle()
	end

	tx, ty, tz = getCreaturePosition(id)
	if tx == nil then
		selfGotoIdle()
	else
		moveToPosition(tx, ty, tz)
	end
end

function selfGotoIdle()
	following = false
	attacking = false
	selfAttackCreature(0)
	target = 0
end

function isPlayerPremiumCallback(cid)
	return isPremium(cid) == TRUE and true or false
end

function msgcontains(message, keyword)
	local a, b = string.find(message, keyword)
	if a == nil or b == nil then
		return false
	end
	return true
end

function selfSayChannel(cid, message)
	return selfSay(message, cid, FALSE)
end

function doPosRemoveItem(_itemid, n, position)
	local thing = getThingfromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if thing.itemid == _itemid then
		doRemoveItem(thing.uid, n)
	else
		return false
	end
	return true
end