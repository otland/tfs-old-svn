local savingEvent = 0

function onSay(cid, words, param)
	if isNumber(param) == TRUE then
		stopEvent(savingEvent)
		save(tonumber(param) * 60 * 1000)
	else
		saveServer()
	end
end

function save(delay)
	saveServer()
	if delay > 0 then
		savingEvent = addEvent(save, delay, delay)
	end
end