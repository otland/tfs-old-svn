local config = {
	item = 1968,
	maxLength = 1024
}

function onSay(cid, words, param, channel)
	doShowTextDialog(cid, config.item, true, config.maxLength)
	registerCreatureEvent(cid, "BanBook")
	return true
end