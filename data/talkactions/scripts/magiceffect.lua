function onSay(cid, words, param)
	if isPlayer(cid) == TRUE and param ~= "" then
		doSendMagicEffect(getCreaturePosition(cid), param)
	end
end