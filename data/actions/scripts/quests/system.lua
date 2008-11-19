local specialQuests = {
	[2001] = 30015 --Annihilator
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local storage = item.uid
	for k,v in ipairs(specialQuests) do
		if(item.actionid == k) then
			storage = v
		end
	end

	local result = "It is empty."
	if(getPlayerStorageValue(cid, storage) == -1) then
		local items = {}
		local reward = 0

		local size = getContainerSize(item.uid)
		if(size == 0) then
			reward = doCopyItem(item, FALSE)
		else
			for i = 0, size do
				local tmp = getContainerItem(item.uid, i)
				if(tmp.itemid > 0) then
					table.insert(items, tmp)
				end
			end
		end

		size = table.maxn(items)
		if(size == 1) then
			reward = doCreateItemEx(items[1].itemid, items[1].type)
		end

		result = "You have found "
		if(reward ~= 0) then
			result = result .. getItemArticle(reward) .. " " .. getItemName(reward) .. "."
		else
			if(size > 20) then
				reward = doCopyItem(item, FALSE)
			elseif(size > 8) then
				reward = doCreateItemEx(1988, 1)
			else
				reward = doCreateItemEx(1987, 1)
			end

			for i = size, 1, -1 do
				local tmp = doCopyItem(items[i], TRUE)
				if(doAddContainerItemEx(reward, tmp.uid) ~= RETURNVALUE_NOERROR) then
					error("[system.lua]: Could not add quest reward!")
					break
				end

				local space = ", "
				if(i == 2) then
					space = " and "
				elseif(i == 1) then
					space = "."
				end

				local ret = getItemDescriptions(tmp.uid)
				if(tmp.type > 0 and isItemRune(tmp.itemid) == TRUE) then
					result = result .. tmp.type .. " charges " .. ret.name .. space
				elseif(tmp.type > 0 and isItemStackable(tmp.itemid) == TRUE) then
					result = result .. tmp.type .. " " .. ret.plural .. space
				else
					result = result .. ret.article .. " " .. ret.name .. space
				end
			end
		end

		if(doPlayerAddItemEx(cid, reward, FALSE) ~= RETURNVALUE_NOERROR) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You don't have enough capacity or free space in backpack for reward.")
			return FALSE
		end

		setPlayerStorageValue(cid, storage, 1)
	end

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, result)
	return TRUE
end
