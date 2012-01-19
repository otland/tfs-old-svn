[ SCRIPTSYSTEM_HELP
	Project Name
		The Forgotten Server

	Version
		0.3.7

	Codenamed
		Crying Damson

	License
		GNU GPLv3

	Forum
		http://otland.net/
]

[ ABOUT
	Few things, about scriptsystem, attributes possible to use in .xml declaration file.
]

[ LIST
	*ACTIONS
		File: actions/actions.xml
		Attributes
			actionid
			uniqueid
			itemid
			fromid, toid
			fromaid, toaid
			fromuid, touid
				Defined in which situations action should be triggered.
				Execution order: unique, action, id, runeid
			allowfaruse
				ItemEx can be used from far?
				Only for items with option 'Use with'.
			blockwalls
				Defines if item will ignore walls and other blocking items.
				Only for items with option 'Use with'.
			function
				Default hardcoded functions.
				Values: increaseItemId, deincreaseItemId
		Function
			onUse(cid, item, fromPosition, itemEx, toPosition)

	*CREATURESCRIPTS
		File: creaturescripts/creaturescripts.xml
		Attributes
			type
				Event type
				Values:
					login, logout,
					spawn-single, spawn-global,
					advance, statschange,
					direction, outfit,
					mailsend, mailchange,
					traderequest, tradeaccept,
					channeljoin, channelleave,
					look, think, textedit, reportbug,
					push, target, follow,
					attack, combat, areacombat, cast
					kill, death, preparedeath,
					channelrequest, reportviolation,
					thankyou, houseedit, throw
		Functions:
			onLogin(cid)
			onLogout(cid, forceLogout)
			onSpawn(cid)
			onAdvance(cid, skill, oldLevel, newLevel)
			onStatsChange(cid, attacker, type, combat, value)
			onDirection(cid, old, current)
			onOutfit(cid, old, current)
			onMailSend(cid, receiver, item, openBox)
			onMailReceive(cid, sender, item, openBox)
			onTradeRequest(cid, target, item)
			onTradeAccept(cid, target, item, targetItem)
			onChannelJoin(cid, channel, users)
			onChannelLeave(cid, channel, users)
			onChannelRequest(cid, channel, custom)
			onLook(cid, thing, position, lookDistance)
			onThink(cid, interval)
			onTextEdit(cid, item, newText)
			onHouseEdit(cid, house, list, text)
			onReportBug(cid, comment)
			onReportViolation(cid, type, reason, name, comment, translation, statementId)
			onThankYou(cid, statementId)
			onAreaCombat(cid, ground, position, aggressive)
			onPush(cid, target, ground, position)
			onTarget(cid, target)
			onFollow(cid, target)
			onCombat(cid, target, aggressive)
			onAttack(cid, target)
			onCast(cid, target)
			onKill(cid, target, damage, flags, war)
			onDeath(cid, corpse, deathList)
			onPrepareDeath(cid, deathList)
			onThrow(cid, item, fromPosition, toPosition)

	*GLOBALEVENTS
		File: globalevents/globalevents.xml
		Attributes
			type
				Server events, script can be executed at server start, shutdown or players record.
				Values:
					startup
					shutdown
					globalsave
					record
			interval
				Script will be executed every x miliseconds.
				Ignored if 'type' specified.
			time
				Script will be executed at specified time.
				For example: '12:00:55'
		Functions
			onThink(interval)
			onStartup()
			onShutdown()
			onGlobalSave
			onRecord(current, old, cid)
			onTime()

	*MOVEMENTS
		File: movements/movements.xml
		Attributes
			type/event
				Values:
					StepIn, StepOut
					Equip, DeEquip
					AddItem, RemoveItem
			slot
				Values:
					head
					necklace
					backpack
					armor
					right-hand, left-hand
					two-handed, hand/shield
					legs
					feet
					ring
					ammo
			tileitem
				Defines if it is a tile item.
			level
				You can specify level required to use this item.
			maglevel
				Magic level required.
			premium
				Premium needed?
				Values: yes/no
			function
				Default hardcoded functions.
				Values:
					onStepInField
					onStepOutField
					onAddField
					onRemoveField
					onEquipItem
					onDeEquipItem
		Functions
			onStepIn(cid, item, position, lastPosition, fromPosition, toPosition, actor)
			onStepOut(cid, item, position, lastPosition, fromPosition, toPosition, actor)
			onEquip(cid, item, slot)
			onDeEquip(cid, item, slot)
			onAddItem(moveItem, tileItem, position, cid)
			onRemoveItem(moveItem, tileItem, position, cid)

	*SPELLS
		File: spells/spells.xml
		Attributes
			//TODO :(
		Function: onCastSpell(cid, var)

	*TALKACTIONS
		File: talkactions/talkactions.xml
		Attributes
			words
				Words used to execute this talkaction.
				Can be separated by a semicolon, fe. words="/a; /b",
					or using own separator, fe. words="/a, /b" separator=","
			separator
				Read "words".
			access
				Access required.
			groups
				Groups allowed to use.
			channel
				Will work only on specified channel.
			filter
				How params should be parsed.
				E.q
					If you use 'quotation' - you will need to write /goto "PlayerName
					If you use 'word' - you will need to write /goto PlayerName
					If you use 'word-spaced' - you are able to make seperate commands with space between word(s)
				Values: quotation/word/word-spaced
			logged
				Defines if talkaction should be logged.
				Default: no
			hidden
				Defines if talkaction should be hidden from displaying it when using /commands
			case-sensitive
				Defines if talkaction should be case sensitive. Is 'no' - then /command will be same as /CoMmAnD
				Default: yes
			exception
				List of players, who can't use this talkaction.
				Example: "GM John;GM 2;Maatt"
		Function: onSay(cid, words, param, channel)

	*WEAPONS
		File: weapons/weapons.xml
		Attributes
			//TODO
		Function: onUseWeapon(cid, var)
]
