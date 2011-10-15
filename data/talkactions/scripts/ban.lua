local config = {
	item = ITEM_ACTION_BOOK,
	maxLength = 1024
}

function onSay(cid, words, param, channel)
	doShowTextDialog(cid, config.item, true, config.maxLength)
	registerCreatureEvent(cid, "BanBook")
	return true
end