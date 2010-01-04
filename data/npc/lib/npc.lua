-- Include the Advanced NPC System
dofile(getDataDir() .. 'npc/lib/npcsystem/npcsystem.lua')

function selfIdle()
	following = false
	attacking = false

	selfAttackCreature(0)
	target = 0
end

function selfSayChannel(cid, message)
	return selfSay(message, cid, false)
end

function selfMoveToCreature(id)
	if(not id or id == 0) then
		return
	end

	local t = getCreaturePosition(id)
	if(not t.x or t.x == nil) then
		return
	end

	selfMoveTo(t.x, t.y, t.z)
	return
end

function getNpcDistanceToCreature(id)
	if(not id or id == 0) then
		selfIdle()
		return nil
	end

	local c = getCreaturePosition(id)
	if(not c.x or c.x == 0) then
		return nil
	end

	local s = getCreaturePosition(getNpcId())
	if(not s.x or s.x == 0 or s.z ~= c.z) then
		return nil
	end

	return math.max(math.abs(s.x - c.x), math.abs(s.y - c.y))
end

function doMessageCheck(message, keyword)
	if(type(keyword) == "table") then
		return table.isStrIn(keyword, message)
	end

	local a, b = message:lower():find(keyword:lower())
	if(a ~= nil and b ~= nil) then
		return true
	end

	return false
end

function doNpcSellItem(cid, itemid, amount, subType, ignoreCap, inBackpacks, backpack)
	local amount, subType, ignoreCap, item = amount or 1, subType or 0, ignoreCap and TRUE or FALSE, 0
	if(isItemStackable(itemid) == TRUE) then
		if(inBackpacks) then
			stuff = doCreateItemEx(backpack, 1)
			item = doAddContainerItem(stuff, itemid, amount)
		else
			stuff = doCreateItemEx(itemid, amount)
		end
		return doPlayerAddItemEx(cid, stuff, ignoreCap) ~= RETURNVALUE_NOERROR and 0 or amount, 0
	end

	local a = 0
	if(inBackpacks) then
		local container, b = doCreateItemEx(backpack, 1), 1
		for i = 1, amount do
			local item = doAddContainerItem(container, itemid, subType)
			if(isInArray({(getContainerCapById(backpack) * b), amount}, i) == TRUE) then
				if(doPlayerAddItemEx(cid, container, ignoreCap) ~= RETURNVALUE_NOERROR) then
					b = b - 1 -- 
					break
				end
				a = i -- a = a + i
				if(amount > i) then  
					container = doCreateItemEx(backpack, 1)
					b = b + 1
				end
			end
		end
		return a, b
	end

	for i = 1, amount do -- normal method for non-stackable items
		local item = doCreateItemEx(itemid, subType)
		if(doPlayerAddItemEx(cid, item, ignoreCap) ~= RETURNVALUE_NOERROR) then
			break
		end
		a = i
	end
	return a, 0
end

function doRemoveItemIdFromPos (id, n, position)
	local thing = getThingFromPos({x = position.x, y = position.y, z = position.z, stackpos = 1})
	if(thing.itemid == id) then
		doRemoveItem(thing.uid, n)
		return true
	end

	return false
end

function getNpcName()
	return getCreatureName(getNpcId())
end

function getNpcPos()
	return getCreaturePosition(getNpcId())
end

function selfGetPosition()
	local t = getNpcPos()
	return t.x, t.y, t.z
end

msgcontains = doMessageCheck
moveToPosition = selfMoveTo
moveToCreature = selfMoveToCreature
selfMoveToPosition = selfMoveTo
selfGotoIdle = selfIdle
isPlayerPremiumCallback = isPremium
doPosRemoveItem = doRemoveItemIdFromPos
doNpcBuyItem = doPlayerRemoveItem
doNpcSetCreatureFocus = selfFocus
getNpcCid = getNpcId
getDistanceTo = getNpcDistanceTo
getDistanceToCreature = getNpcDistanceToCreature

if not eventDelayedSay then eventDelayedSay = {} end

local func = function(pars)
	if isCreature(pars.cid) == TRUE and isPlayer(pars.pcid) == TRUE then
		doCreatureSay(pars.cid, pars.text, pars.type, false, pars.pcid, getCreaturePosition(pars.cid))
	end
end

function doCreatureSayWithDelay(cid, text, type, delay, e, pcid)
	if isCreature(cid) == TRUE and isPlayer(pcid) == TRUE then
		e.event = addEvent(func, delay < 1 and 1000 or delay, {cid=cid, text=text, type=type, e=e, pcid=pcid})
	end
end

function cancelNPCTalk(events)
	for i = 1, #events do
		stopEvent(events[i].event)
	end
	events = nil
end

function doNPCTalkALot(msgs, interval, pcid)
	if eventDelayedSay[pcid] then
		cancelNPCTalk(eventDelayedSay[pcid])
	end
	if isPlayer(pcid) == TRUE then
		eventDelayedSay[pcid] = {}
		local ret = {}
		for i = 1, #msgs do
			eventDelayedSay[pcid][i] = {}
			doCreatureSayWithDelay(getNpcCid(), msgs[i], TALKTYPE_PRIVATE_NP, ((i-1) * (interval or 10000)) + 1000, eventDelayedSay[pcid][i], pcid)
			table.insert(ret, eventDelayedSay[pcid][i])
		end
		return(ret)
	end
end