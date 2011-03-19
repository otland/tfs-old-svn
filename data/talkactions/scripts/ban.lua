function onSay(cid, words, param, channel)
	doShowTextDialog(cid, 1968, true, 1024)
	registerCreatureEvent(cid, "BanBook")
	return true
end