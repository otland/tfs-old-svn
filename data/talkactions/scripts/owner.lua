function onSay(cid, words, param)
	local guid = getPlayerGUIDByName(param)
	local hid = getTileHouseInfo(getCreaturePosition(cid))
	if(hid and guid) then
		setHouseOwner(hid, guid, TRUE)
	end
	return TRUE
end
