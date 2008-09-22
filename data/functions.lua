function doPlayerGiveItem(cid, itemid, count, charges)
	local hasCharges = (isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE)
	if(hasCharges and charges == nil) then
		charges = 1
	end
	while count > 0 do
		local tempcount = 1
		if(hasCharges) then
			tempcount = charges
		end
		if(isItemStackable(itemid) == TRUE) then
			tempcount = math.min(100, count)
   		end
	   	local ret = doPlayerAddItem(cid, itemid, tempcount)
	   	if(ret == LUA_ERROR) then
			ret = doCreateItem(itemid, tempcount, getPlayerPosition(cid))
		end
		if(ret ~= LUA_ERROR) then
			if(hasCharges) then
				count = count - 1
			else
				count = count - tempcount
			end
		else
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end

function doPlayerTakeItem(cid, itemid, count)
	if(getPlayerItemCount(cid,itemid) >= count) then
		while count > 0 do
			local tempcount = 0
			if(isItemStackable(itemid) == TRUE) then
				tempcount = math.min(100, count)
			else
				tempcount = 1
			end
			local ret = doPlayerRemoveItem(cid, itemid, tempcount)
			if(ret ~= LUA_ERROR) then
				count = count - tempcount
			else
				return LUA_ERROR
			end
		end
		if(count == 0) then
			return LUA_NO_ERROR
		end
	else
		return LUA_ERROR
	end
end

function doPlayerBuyItem(cid, itemid, count, cost, charges)
	if(doPlayerRemoveMoney(cid, cost) == TRUE) then
		return doPlayerGiveItem(cid, itemid, count, charges)
	end
	return LUA_ERROR
end

function doPlayerBuyItemContainer(cid, container, itemid, count, cost, charges)
	if doPlayerRemoveMoney(cid, cost) == TRUE then
		for i = 1, count do
			local containerItem = doPlayerAddItem(cid, container, 1)
			for x = 1, getContainerCap(containerItem[1]) do
				doAddContainerItem(containerItem[1], itemid, charges)
			end
		end
		return LUA_NO_ERROR
	end
	return LUA_ERROR
end

function doPlayerSellItem(cid, itemid, count, cost)
	if(doPlayerTakeItem(cid, itemid, count) == LUA_NO_ERROR) then
		if(doPlayerAddMoney(cid, cost) ~= TRUE) then
			error('Could not add money to ' .. getPlayerName(cid) .. ' (' .. cost .. 'gp)')
		end
		return LUA_NO_ERROR
	end
	return LUA_ERROR
end

function isInRange(pos, fromPos, toPos)
	return (pos.x >= fromPos.x and pos.y >= fromPos.y and pos.z >= fromPos.z and pos.x <= toPos.x and pos.y <= toPos.y and pos.z <= toPos.z) and TRUE or FALSE
end

function isPremium(cid)
	return (isPlayer(cid) == TRUE and (getPlayerPremiumDays(cid) > 0 or getConfigInfo('freePremium') == "yes")) and TRUE or FALSE
end

function getMonthDayEnding(day)
	if day == "01" or day == "21" or day == "31" then
		return "st"
	elseif day == "02" or day == "22" then
		return "nd"
	elseif day == "03" or day == "23" then
		return "rd"
	else
		return "th"
	end
end

function getMonthString(m)
	return os.date("%B", os.time{year = 1970, month = m, day = 1})
end

function getArticle(str)
	return str:find("[AaEeIiOoUuYy]") == 1 and "an" or "a"
end

function isNumber(str)
	return tonumber(str) ~= nil and TRUE or FALSE
end

function getDistanceBetween(firstPosition, secondPosition)
	local xDif = math.abs(firstPosition.x - secondPosition.x)
	local yDif = math.abs(firstPosition.y - secondPosition.y)

	local posDif = math.max(xDif, yDif)
	if(firstPosition.z ~= secondPosition.z) then
		posDif = posDif + 9 + 6
	end
	return posDif
end

function doPlayerAddAddons(cid, addon)
	for i = 0, table.maxn(maleOutfits) do
		doPlayerAddOutfit(cid, maleOutfits[i], addon)
	end

	for i = 0, table.maxn(femaleOutfits) do
		doPlayerAddOutfit(cid, femaleOutfits[i], addon)
	end
end

function isSorcerer(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isSorcerer: Player not found.")
		return false
	end

	return (isInArray({1,5}, getPlayerVocation(cid)) == TRUE)
end

function isDruid(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isDruid: Player not found.")
		return false
	end

	return (isInArray({2,6}, getPlayerVocation(cid)) == TRUE)
end

function isPaladin(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isPaladin: Player not found.")
		return false
	end

	return (isInArray({3,7}, getPlayerVocation(cid)) == TRUE)
end

function isKnight(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isKnight: Player not found.")
		return false
	end

	return (isInArray({4,8}, getPlayerVocation(cid)) == TRUE)
end

function isRookie(cid)
	if(isPlayer(cid) == FALSE) then
		debugPrint("isRookie: Player not found.")
		return false
	end

	return (isInArray({0}, getPlayerVocation(cid)) == TRUE)
end

function getConfigInfo(info)
	if (type(info) ~= 'string') then return nil end

	dofile(getConfigFile())
	return _G[info]
end

function getDirectionTo(pos1, pos2)
	local dir = NORTH
	if(pos1.x > pos2.x) then
		dir = WEST
		if(pos1.y > pos2.y) then
			dir = NORTHWEST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHWEST
		end
	elseif(pos1.x < pos2.x) then
		dir = EAST
		if(pos1.y > pos2.y) then
			dir = NORTHEAST
		elseif(pos1.y < pos2.y) then
			dir = SOUTHEAST
		end
	else
		if(pos1.y > pos2.y) then
			dir = NORTH
		elseif(pos1.y < pos2.y) then
			dir = SOUTH
		end
	end
	return dir
end

function getPlayerLookPos(cid)
	return getPosByDir(getThingPos(cid), getPlayerLookDir(cid))
end

function getPosByDir(fromPosition, direction, size)
	local n = size or 1

	local pos = fromPosition
	if(direction == NORTH) then
		pos.y = pos.y - n
	elseif(direction == SOUTH) then
		pos.y = pos.y + n
	elseif(direction == WEST) then
		pos.x = pos.x - n
	elseif(direction == EAST) then
		pos.x = pos.x + n
	elseif(direction == NORTHWEST) then
		pos.y = pos.y - n
		pos.x = pos.x - n
	elseif(direction == NORTHEAST) then
		pos.y = pos.y - n
		pos.x = pos.x + n
	elseif(direction == SOUTHWEST) then
		pos.y = pos.y + n
		pos.x = pos.x - n
	elseif(direction == SOUTHEAST) then
		pos.y = pos.y + n
		pos.x = pos.x + n
	end

	return pos
end

function getPlayerMoney(cid)
	return ((getPlayerItemCount(cid, ITEM_CRYSTAL_COIN) * 10000) + (getPlayerItemCount(cid, ITEM_PLATINUM_COIN) * 100) + getPlayerItemCount(cid, ITEM_GOLD_COIN))
end

function doPlayerWithdrawAllMoney(cid)
	return doPlayerWithdrawMoney(cid, getPlayerBalance(cid))
end

function doPlayerDepositAllMoney(cid)
	return doPlayerDepositMoney(cid, getPlayerMoney(cid))
end

function doPlayerTransferAllMoneyTo(cid, target)
	return doPlayerTransferMoneyTo(cid, target, getPlayerBalance(cid))
end

function playerExists(name)
	return (getPlayerGUIDByName(name) ~= 0)
end

function getTibiaTime()
	local worldTime = getWorldTime()
	local hours = 0
	while (worldTime > 60) do
		hours = hours + 1
		worldTime = worldTime - 60
	end

	return {hours = hours, minutes = worldTime}
end

function doWriteLogFile(file, text)
	local file = io.open(file, "a+")
	file:write("[" .. os.date("%d/%m/%Y  %H:%M:%S") .. "] " .. text .. "\n")
	file:close()
end

function isInArea(pos, fromPos, toPos)
	if pos.x >= fromPos.x and pos.x <= toPos.x then
		if pos.y >= fromPos.y and pos.y <= toPos.y then
			if pos.z >= fromPos.z and pos.z <= toPos.z then
				return TRUE
			end
		end
	end
	return FALSE
end

function getExperienceForLevel(level)
	return (50 * level * level * level) / 3 - 100 * level * level + (850 * level) / 3 - 200
end

exhaustion =
{

	check = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)
		if (os.time(t) >= exhaust) then
			return FALSE
		else
			return TRUE
		end
	end,

	get = function (cid, storage)
		local exhaust = getPlayerStorageValue(cid, storage)
		local left = exhaust - os.time(t)
		if (left >= 0) then
			return left
		else
			return FALSE
		end
	end,

	set = function (cid, storage, time)
		setPlayerStorageValue(cid, storage, os.time(t) + time)
	end,

	make = function (cid, storage, time)
		local exhaust = exhaustion.get(cid, storage)
		if (exhaust > 0) then
			return FALSE
		else
			exhaustion.set(cid, storage, time)
			return TRUE
		end
	end
}

	table.find = function (table, value)
		for i, v in pairs(table) do
			if(v == value) then
				return i
			end
		end
		return nil
	end
	table.getPos = table.find


	table.isStrIn = function (txt, str)
		local result = false
		for i, v in pairs(str) do
			result = (string.find(txt, v) and not string.find(txt, '(%w+)' .. v) and not string.find(txt, v .. '(%w+)'))
			if(result) then
				break
			end
		end
		return result
	end

	table.countElements = function (table, item)
		local count = 0
		for i, n in pairs(table) do
			if(item == n) then
				count = count + 1
			end
		end
		return count
	end

	table.getCombinations = function (table, num)
		local a, number, select, newlist = {}, #table, num, {}
		for i = 1, select do
			a[#a + 1] = i
		end

		local newthing = {}
		while(TRUE) do
			local newrow = {}
			for i = 1, select do
				newrow[#newrow + 1] = table[a[i]]
			end

			newlist[#newlist + 1] = newrow
			i = select
			while(a[i] == (number - select + i)) do
				i = i - 1
			end

			if(i < 1) then
				break
			end

			a[i] = a[i] + 1
			for j = i, select do
				a[j] = a[i] + j - i
			end
		end
		return newlist
	end

	string.split = function (str)
		local t = {}
		local function helper(word)
			table.insert(t, word)
			return ""
		end

		if(not str:gsub("%w+", helper):find"%S") then
			return t
		end
	end

	string.trim = function (str)
		return (string.gsub(str, "^%s*(.-)%s*$", "%1"))
	end

	string.explode = function (str, sep)
		local pos, t = 1, {}
		if #sep == 0 or #str == 0 then
			return
		end

		for s, e in function() return string.find(str, sep, pos) end do
			table.insert(t, string.trim(string.sub(str, pos, s - 1)))
			pos = e + 1
		end

		table.insert(t, string.trim(string.sub(str, pos)))
		return t
	end