function onSay(cid, words, param)
	if param ~= nil then
		doPlayerBroadcastMessage(cid, param)
	end
	return TRUE
end
