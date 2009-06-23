local config = {
	storage = 30001,
	firstItems = {2050, 2382}
}

function onLogin(cid)
	if(getPlayerStorageValue(cid, config.storage) > 0) then
		return true
	end

	for i = 1, #config.firstItems do
		doPlayerAddItem(cid, config.firstItems[i], 1)
	end

	if(getPlayerSex(cid) == PLAYERSEX_FEMALE) then
		doPlayerAddItem(cid, 2651, 1)
	else
		doPlayerAddItem(cid, 2650, 1)
	end

	doAddContainerItem(doPlayerAddItem(cid, 1987, 1), 2674, 1)
	setPlayerStorageValue(cid, config.storage, 1)
 	return true
end
