local annihilatorReward = {1990, 2400, 2431, 2494}
function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.uid > 1000 and item.uid < 10000) then
		local result = "It is empty."
		if(isInArray(annihilatorReward, item.uid) == TRUE) then
			if(getPlayerStorageValue(cid, 30015) == -1) then
				result = "You have found " .. getItemArticleById(item.uid) .. " " .. getItemNameById(item.uid)
				local ret = getItemWeightById(item.uid, 1)
				if(getPlayerFreeCap(cid) >= ret) then
					local tmp = doPlayerAddItem(cid, item.uid, 1)
					if(item.uid == 1990) then
						doAddContainerItem(tmp[1], 2326, 1)
					end

					result = result .. "."
					setPlayerStorageValue(cid, 30015, 1)
				else
					result = result .. " weighing " .. ret .. " oz. It is too heavy."
				end
			end
		elseif(getPlayerStorageValue(cid, item.uid) == -1) then
			local items = {}
			for i = 1, getContainerSize(item.uid) do
				local tmp = getContainerItem(item.uid, i)
				if(item.itemid > 0) then
					table.insert(items, item)
				end
			end

			local reward = 0
			if(#items == 0) then
				reward = doCopyItem(item, FALSE)
			elseif(#items == 1) then
				reward = doCreateItemEx(items[1].itemid, items[1].type)
			end

			result = "You have found "
			if(reward ~= 0) then
				result = result .. getItemArticle(reward) .. " " .. getItemName(reward) .. "."
			else
				if(#items > 20) then
					reward = doCopyItem(item, FALSE)
				elseif(#items > 8)
					reward = doCreateItemEx(1988, 1)
				else
					reward = doCreateItemEx(1987, 1)
				end

				for i = #items, 1, -1 do
					local tmp = doCopyItem(items[i], TRUE)
					if(doAddContainerItemEx(reward, tmp) ~= RETURNVALUE_NOERROR) then
						error("[quests.lua]: Could not add quest reward!")
						break
					end

					local ret = ", "
					if(i == 2) then
						ret = " and "
					else if(i == 1) then
						ret = "."
					end

					result = result .. getItemArticle(item) .. " " .. getItemName(item) .. ret
				end
			end

			if(doPlayerAddItemEx(cid, reward, FALSE) ~= RETURNVALUE_NOERROR) then
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "You don't have enough capacity or free space in backpack for reward.")
				return FALSE
			end

			setPlayerStorageValue(cid, item.uid, 1)
		end

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, result)
		return TRUE
	end

	return FALSE
end
