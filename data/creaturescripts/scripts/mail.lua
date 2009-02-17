function onReceiveMail(cid, sender, item, openBox)
	if(openDepot == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "New mail has arrived.")
	end
end
