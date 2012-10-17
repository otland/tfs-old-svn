modaldialog = {
	title = "title",
	message = "message",
	buttons = {
		{ id = 1, value = "okay" },
		{ id = 2, value = "cancel" },
	},
	buttonEnter = 1,
	buttonEscape = 2,
	choices = {
		{ id = 1, value = "add 500 exp" },
		{ id = 2, value = "remove 500 exp" }
	},
	popup = true
}


function callback(cid, button, choice)
	--anything u want
	if (button == 1) then
		local exp = 0
		if (choice == 1) then
			exp = 500
		end
		if (choice == 2) then
			exp = -500
		end
		doPlayerAddExperience(cid, exp);
	end
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
	--struct, id, creature id, callback
	--NOTE:
	--	id should be unique id of all dialogs in whole scripts
	addDialog(modaldialog, 2345, cid, callback);
end