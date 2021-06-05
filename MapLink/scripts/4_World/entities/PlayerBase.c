modded class PlayerBase extends ManBase{
	
	protected bool m_MapLink_UnderProtection = false;
	
	protected string m_TransferPoint = "";
	protected ref Timer m_MapLink_UnderProtectionTimer;
	
	
	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableBool("m_MapLink_UnderProtection");
	}
	
	bool IsUnderMapLinkProtection(){
		return (m_MapLink_UnderProtection);
	}
	
	
	bool UApiSaveTransferPoint(string point = ""){
		m_TransferPoint = point;
		return true;
	}
	
	
	bool IsBeingTransfered(){
		return (m_TransferPoint != "");
	}
	
	protected void UpdateMapLinkProtectionClient(int time){
		Print("[MAPLINK] UpdateMapLinkProtectionClient" + time);
		if (time > 0){
			GetDayZGame().MapLinkStartCountDown(time);		
			GetInputController().OverrideRaise(true, false);
			GetInputController().OverrideMeleeEvade(true, false);
		}
		if (time <= 0){
			GetDayZGame().MapLinkStopCountDown();		
			GetInputController().OverrideMeleeEvade(false, false);
			GetInputController().OverrideRaise(false, false);
		}
	}
	
	
	void UpdateMapLinkProtection(int time = -1){
		Print("[MAPLINK] UpdateMapLinkProtection" + time);
		if (!GetGame().IsServer()){return;}
		RPCSingleParam(MAPLINK_UNDERPROTECTION, new Param1<int>(time), true, GetIdentity());
		if (m_MapLink_UnderProtection && time < 0){
			GetInputController().OverrideMeleeEvade(false, false);
			GetInputController().OverrideRaise(false, false);
			if (m_MapLink_UnderProtectionTimer){
				if (m_MapLink_UnderProtectionTimer.IsRunning()){
					m_MapLink_UnderProtectionTimer.Stop();
				}
			}
			RemoveProtectionSafe();
			return;
		}
		SetAllowDamage(false);
		m_MapLink_UnderProtection = true;
		GetInputController().OverrideMeleeEvade(true, false);
		GetInputController().OverrideRaise(true, false);
		
		SetSynchDirty();
		
		if (!m_MapLink_UnderProtectionTimer){
			m_MapLink_UnderProtectionTimer = new Timer;
		}
		if (m_MapLink_UnderProtectionTimer.IsRunning()){
			m_MapLink_UnderProtectionTimer.Stop();
		}
		m_MapLink_UnderProtectionTimer.Run(time, this, "UpdateMapLinkProtection", new Param1<int>(-1), false);
	}

	protected void RemoveProtectionSafe(){
		bool PlayerHasGodMode = false;
		#ifdef JM_COT
			if ( GetGame().IsServer() && m_JMHasGodMode ){
				Print("[MAPLINK] RemoveProtectionSafe COT ADMIN TOOLS ACTIVE");
				PlayerHasGodMode = true;
			}
		#endif
		#ifdef VPPADMINTOOLS
			if ( GetGame().IsServer() && hasGodmode ){
				Print("[MAPLINK] RemoveProtectionSafe VPP ADMIN TOOLS ACTIVE");
				PlayerHasGodMode = true;
			}
		#endif
		#ifdef ZOMBERRY_AT
			if ( GetGame().IsServer() && ZBGodMode ){
				Print("[MAPLINK] RemoveProtectionSafe ZOMBERRY ADMIN TOOLS ACTIVE");
				PlayerHasGodMode = true;
			}
		#endif
		#ifdef TRADER 
			if (GetGame().IsServer() && m_Trader_IsInSafezone){
				Print("[MAPLINK] RemoveProtectionSafe Player Is In Trader(DrJones) Safe Zone");
				PlayerHasGodMode = true;
			}
		#endif
		#ifdef TRADERPLUS
			if (GetGame().IsServer() && IsInsideSZ && IsInsideSZ.SZStatut){
				Print("[MAPLINK] RemoveProtectionSafe Player Is In Trader+ Safe Zone");
				PlayerHasGodMode = true;
			}		
		#endif
		if (!PlayerHasGodMode && GetGame().IsServer() ){
			SetAllowDamage(true);
		}
		m_MapLink_UnderProtection = false;
		SetSynchDirty();
	}
	
	
    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
		//Making sure not to save freshspawns or dead people, dead people logic is handled in EEKilled
		if (!GetGame().IsClient() && GetHealth("","Health") > 0 && StatGet(AnalyticsManagerServer.STAT_PLAYTIME) >= MAPLINK_BODYCLEANUPTIME ){ 
			this.SavePlayerToUApi();
		}
    }
	
	void SavePlayerToUApi(){
		if (this.GetIdentity() && GetGame().IsServer()){
			GetGame().AdminLog("[MAPLINK] Saving Player Data to API: " + GetIdentity().GetName() + " (" + GetIdentity().GetId() +  ")" );
			autoptr PlayerDataStore teststore = new PlayerDataStore(PlayerBase.Cast(this));
			UApi().db(PLAYER_DB).Save("MapLink", this.GetIdentity().GetId(), teststore.ToJson());
			delete teststore;
			//NotificationSystem.SimpleNoticiation(" You're Data has been saved to the API", "Notification","Notifications/gui/data/notifications.edds", -16843010, 10, this.GetIdentity());
		}
	}
	
	void OnUApiSave(ref PlayerDataStore data){
		int i = 0;
		data.m_TimeSurvivedValue = StatGet(AnalyticsManagerServer.STAT_PLAYTIME);
		data.m_PlayersKilledValue = StatGet(AnalyticsManagerServer.STAT_PLAYERS_KILLED);
		data.m_InfectedKilledValue = StatGet(AnalyticsManagerServer.STAT_INFECTED_KILLED);
		data.m_DistanceTraveledValue = StatGet(AnalyticsManagerServer.STAT_DISTANCE);
		data.m_LongRangeShotValue = StatGet(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT );
		data.m_LifeSpanState = GetLifeSpanState();
		data.m_LastShavedSeconds = GetLastShavedSeconds();
		data.m_BloodType = GetStatBloodType().Get();
		data.m_HasBloodTypeVisible = HasBloodyHands();
		data.m_HasBloodyHandsVisible = HasBloodTypeVisible();
		for(i = 0; i < m_ModifiersManager.m_ModifierList.Count(); i++){
            ModifierBase mdfr = ModifierBase.Cast(m_ModifiersManager.m_ModifierList.GetElement(i));
            if (mdfr && mdfr.IsActive() && mdfr.IsPersistent()) { 
				data.AddModifier( mdfr.GetModifierID(), mdfr.GetAttachedTime());
			}
		}
		for(i = 0; i < m_AgentPool.m_VirusPool.Count();i++){
			data.AddAgent(m_AgentPool.m_VirusPool.GetKey(i), m_AgentPool.m_VirusPool.GetElement(i));
		}
		data.m_TransferPoint = m_TransferPoint;
		data.m_BrokenLegState = m_BrokenLegState;
		
		data.m_BleedingBits = GetBleedingBits();
		if (GetBleedingManagerServer()){
			GetBleedingManagerServer().OnUApiSave(data);
		} else {
			Print("[MAPLINK] Bleeding Manager is NULL");
		}
		if (m_PlayerStomach){
			for (i = 0; i < m_PlayerStomach.m_StomachContents.Count(); i++){
				StomachItem stomachItem;
				if (Class.CastTo(stomachItem, m_PlayerStomach.m_StomachContents.Get(i))){
					data.AddStomachItem(stomachItem.m_Amount, stomachItem.m_FoodStage, stomachItem.m_ClassName, stomachItem.m_Agents);
				}
			}
		}	
	}
	
	void OnUApiLoad(ref PlayerDataStore data){
		int i = 0;
		
		StatUpdate(AnalyticsManagerServer.STAT_PLAYTIME, data.m_TimeSurvivedValue );
		StatUpdate(AnalyticsManagerServer.STAT_PLAYERS_KILLED, data.m_PlayersKilledValue);
		StatUpdate(AnalyticsManagerServer.STAT_INFECTED_KILLED, data.m_InfectedKilledValue);
		StatUpdate(AnalyticsManagerServer.STAT_DISTANCE, data.m_DistanceTraveledValue);
		StatUpdate(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT, data.m_LongRangeShotValue );
		SetLifeSpanStateVisible(data.m_LifeSpanState);
		SetLastShavedSeconds(data.m_LastShavedSeconds);
		SetBloodyHands(data.m_HasBloodyHandsVisible);
		for (i = 0; i < data.m_Modifiers.Count(); i++){
			if (data.m_Modifiers.Get(i)){
				ModifierBase mdfr = m_ModifiersManager.GetModifier(data.m_Modifiers.Get(i).ID());
				if (mdfr.IsTrackAttachedTime() && data.m_Modifiers.Get(i).Value() >= 0){
					mdfr.SetAttachedTime(data.m_Modifiers.Get(i).Value());
				}
				m_ModifiersManager.ActivateModifier(data.m_Modifiers.Get(i).ID(), EActivationType.TRIGGER_EVENT_ON_CONNECT);
			}
		}
		for(i = 0; i < data.m_Agents.Count();i++){
			m_AgentPool.SetAgentCount(data.m_Agents.Get(i).ID(), data.m_Agents.Get(i).Value());
		}
		data.m_TransferPoint = "";
		m_TransferPoint = "";
		m_BrokenLegState = data.m_BrokenLegState;
		//SetBleedingBits(data.m_BleedingBits);
		if (GetBleedingManagerServer()){	
			GetBleedingManagerServer().OnUApiLoad(data);
		} else {
			Print("[MAPLINK] Bleeding Manager is NULL");
		}
		GetStatWet().Set(data.m_Stat_Wet);
		GetStatSpecialty().Set(data.m_Stat_Specialty);
		GetStatHeatBuffer().Set(data.m_Stat_HeatBuffer);
		GetStatStamina().Set(data.m_Stat_Stamina);
		GetStatToxicity().Set(data.m_Stat_Toxicity);
		GetStatWater().Set(data.m_Stat_Water);
		GetStatEnergy().Set(data.m_Stat_Energy);
		GetStatBloodType().Set(data.m_BloodType);
		
		SetBloodType(data.m_BloodType);
		SetBloodTypeVisible(data.m_HasBloodTypeVisible);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.SetSynchDirty);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendUApiAfterLoadClient, 200);
		if (m_PlayerStomach && data.m_Stomach){
			for (i = 0; i < data.m_Stomach.Count(); i++){
				UApiStomachItem stomachItem;
				if (Class.CastTo(stomachItem, data.m_Stomach.Get(i))){
					m_PlayerStomach.AddToStomach(stomachItem.m_ClassName, stomachItem.m_Amount, stomachItem.m_FoodStage, stomachItem.m_Agents );
				}
			}
		}	
	}
	
	void SendUApiAfterLoadClient(){
		RPCSingleParam(MAPLINK_AFTERLOADCLIENT, new Param1<bool>( true ), true, GetIdentity());
	}
	
	override void OnDisconnect(){
		//If the player has played less than 1 minutes just kill them so their data doesn't save to the local database
		if ( StatGet(AnalyticsManagerServer.STAT_PLAYTIME) <= MAPLINK_BODYCLEANUPTIME){ 
			SetHealth("","", 0); 
		}
		super.OnDisconnect();
	}
	
	
	override void EEKilled( Object killer )
	{
		//Only save dead people who've been on the server for more than 1 minutes and who arn't tranfering
		if ( ( !IsBeingTransfered() && StatGet(AnalyticsManagerServer.STAT_PLAYTIME) > MAPLINK_BODYCLEANUPTIME ) || ( killer && killer != this )){
			this.SavePlayerToUApi();
		}
		//If they are transfering delete
		if ( IsBeingTransfered()  && ( !killer || killer == this )){
			GetGame().AdminLog("[MAPLINK] Deleteing Player: " + GetIdentity().GetName() + " (" + GetIdentity().GetId() +  ") cause of transfer" );
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Delete, 50, false);
		}
		
		//Fresh spawn just delete the body since I have to spawn players in to send notifications about player transfers
		if ( !IsBeingTransfered() && StatGet(AnalyticsManagerServer.STAT_PLAYTIME) <= MAPLINK_BODYCLEANUPTIME && ( !killer || killer == this )){
			GetGame().AdminLog("[MAPLINK] Deleteing Player: " + GetIdentity().GetName() + " (" + GetIdentity().GetId() +  ") cause they are fresh spawn" );
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Delete, 50, false);
		}
		
		super.EEKilled( killer );
	}

	
	void UApiKillAndDeletePlayer(){
		if (GetIdentity()){
			GetGame().AdminLog("[MAPLINK] Killing for transfering Player: " + GetIdentity().GetName() + " (" + GetIdentity().GetId() +  ")" );
		}
		SetAllowDamage(true);
		SetHealth("","", 0);
	}
	
	void UApiRequestTravel(string arrivalPoint, string serverName ){
		if (GetGame().IsClient()){
			RPCSingleParam(MAPLINK_REQUESTTRAVEL, new Param2<string, string>(arrivalPoint,  serverName), true, NULL);
		}
		if (GetGame().IsServer()){
			UApiDoTravel(arrivalPoint, serverName);
		}
	}
	
	protected void UApiDoTravel(string arrivalPoint, string serverName){
		UApiServerData serverData = UApiServerData.Cast( GetMapLinkConfig().GetServer( serverName ) );
		MapLinkDepaturePoint dpoint = MapLinkDepaturePoint.Cast( GetMapLinkConfig().GetDepaturePoint( GetPosition() ) );
		int cost;
		int id;
		if (dpoint && serverData && dpoint.GetArrivalPointData(arrivalPoint, id, cost) && GetIdentity() && (cost <= 0 || MLGetPlayerBalance(id) >= cost)){
			MLRemoveMoney(id,cost);
			this.UApiSaveTransferPoint(arrivalPoint);
			this.SavePlayerToUApi();
			Print("[MAPLINK] Do Travel Verified for " + GetIdentity().GetId() +  " Sending to Server: " + serverName  + " at ArrivalPoint: " + arrivalPoint );
			GetGame().AdminLog("[MAPLINK]  Player: " + GetIdentity().GetName() + " (" + GetIdentity().GetId() +  ") Sending to Server: " + serverName  + " at ArrivalPoint: " + arrivalPoint );
			GetRPCManager().SendRPC("MapLink", "RPCRedirectedKicked", new Param1<UApiServerData>(serverData), true, GetIdentity());
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UApiKillAndDeletePlayer, 350, false);
		} else {
			string pid = "NULL";
			if (GetIdentity()){
				pid = GetIdentity().GetId();
			}
			Error("[MAPLINK] User " + pid + " Tried to travel to " + arrivalPoint + " on " + serverName + " but validation failed");
		}
			
	} 
	
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		
		if (rpc_type == MAPLINK_AFTERLOADCLIENT && GetGame().IsClient()) {
			Param1<bool> alcdata;
			if (ctx.Read(alcdata))	{
				if (alcdata.param1){
					UApiAfterLoadClient();
				}
			}
		}
		if (rpc_type == MAPLINK_UNDERPROTECTION && GetGame().IsClient()) {
			Param1<int> updata;
			if (ctx.Read(updata))	{
				UpdateMapLinkProtectionClient(updata.param1);
			}
		}
		
		if ( rpc_type == MAPLINK_REQUESTTRAVEL && sender && GetIdentity() && GetGame().IsServer() ){
			Param2<string, string> rtdata;
			if (ctx.Read(rtdata) && GetIdentity().GetId() == sender.GetId())	{
				UApiDoTravel(rtdata.param1, rtdata.param2);
			}
		}
	}
	
	void UApiAfterLoadClient(){
		this.UpdateInventoryMenu();
	}
	
	
	
	override void SetSuicide(bool state)
	{
		super.SetSuicide(state);

		if (state && IsUnderMapLinkProtection() && GetGame().IsServer()){
			SetAllowDamage(true);
		}
	}
	
	
	void SetActions(out TInputActionMap InputActionMap) {
		
		super.SetActions(InputActionMap);
		AddAction(ActionMapLinkOpenOnAny, InputActionMap);
		
	}
	
	bool FindDepaturePointForEntity(EntityAI entity, out MapLinkDepaturePoint depaturePoint){
		if (!entity)
			return false;
		
		if (!depaturePoint){ //So you can use super here and if the point is set don't set it.
			if (GetMapLinkConfig().IsDepaturePoint(entity.GetType(), entity.GetPosition())){
				depaturePoint = MapLinkDepaturePoint.Cast(GetMapLinkConfig().GetDepaturePointAny(entity.GetType(), entity.GetPosition()));
				return true;
			}
			return false;
		}
		return true;
	}
	
	
	
	
	
	
	//Money Handling used from Hived Banking
	bool MLCanAccept(int ID, ItemBase item){
		return !item.IsRuined() || GetMapLinkConfig().GetCurrency(ID).CanUseRuinedBills;
	}
	
	
	int MLGetPlayerBalance(int ID){
		int PlayerBalance = 0;
		if (!GetMapLinkConfig().GetCurrency(ID) || GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count() < 1){
			Error("[MAPLINK] Currency ID: " + ID + " is not configured");
			return 0;
		}
		array<EntityAI> inventory = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventory);
		
		ItemBase item;
		for (int i = 0; i < inventory.Count(); i++){
			if (Class.CastTo(item, inventory.Get(i))){
				for (int j = 0; j < GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count(); j++){
					if (item.GetType() == GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Item && MLCanAccept(ID, item)){
						PlayerBalance += MLCurrentQuantity(item) * GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Value;
					}
				}
			}
		}
		return PlayerBalance;
	}
	
	
	int MLAddMoney(int ID, int Amount){
		if (Amount <= 0){
			return 2;
		}
		int Return = 0;
		int AmountToAdd = Amount;
		int LastIndex = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count() - 1;
		int SmallestCurrency = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(LastIndex).Value;
		bool NoError = true;
		int PlayerBalance = MLGetPlayerBalance(ID);
		int OptimalPlayerBalance = PlayerBalance + AmountToAdd;
		
		MapLinkMoneyValue MoneyValue = GetMapLinkConfig().GetCurrency(ID).GetHighestDenomination(AmountToAdd);
		int MaxLoop = 3000;
		while (MoneyValue && AmountToAdd >= SmallestCurrency && NoError && MaxLoop > 0){
			MaxLoop--;
			int AmountToSpawn = GetMapLinkConfig().GetCurrency(ID).GetAmount(MoneyValue,AmountToAdd);
			if (AmountToSpawn == 0){
				NoError = false;
			}
			
			int AmountLeft = MLCreateMoneyInventory(MoneyValue.Item, AmountToSpawn);
			if (AmountLeft > 0){
				Return = 1;
				MLCreateMoneyGround(MoneyValue.Item, AmountLeft);
			}
			
			int AmmountAdded = MoneyValue.Value * AmountToSpawn;
			
			AmountToAdd = AmountToAdd - AmmountAdded;
			
			MapLinkMoneyValue NewMoneyValue = GetMapLinkConfig().GetCurrency(ID).GetHighestDenomination(AmountToAdd);
			if (NewMoneyValue && NewMoneyValue != MoneyValue){
				MoneyValue = NewMoneyValue;
			} else {
				NoError = false;
			}
		}
		return Return;
	}
	
	
	int MLRemoveMoney(int ID, int Amount){
		if (Amount <= 0){
			return 2;
		}
		int Return = 0;
		int AmountToRemove = Amount;
		int LastIndex = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count() - 1;
		int SmallestCurrency = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(LastIndex).Value;
		bool NoError = true;
		for (int i = 0; i < GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count(); i++){
			AmountToRemove =  MLRemoveMoneyInventory(ID, GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(i), AmountToRemove);
		}
		if (AmountToRemove >= SmallestCurrency){ // Now to delete a larger bill and make change
			for (int j = LastIndex; j >= 0; j--){
				//Print("[MapLink] Trying to remove " + GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Item);
				int NewAmountToRemove =  MLRemoveMoneyInventory(ID, GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j), GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Value);
				if (NewAmountToRemove == 0){
					int AmountToAddBack = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Value - AmountToRemove;
					//Print("[MapLink] A " + GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(j).Item + " removed trying to add back " + AmountToAddBack );
					Return = MLAddMoney(ID, AmountToAddBack);
				}
			}
		}
		return Return;
	}
	
	//Return how much left still to remove
	int MLRemoveMoneyInventory(int ID, MapLinkMoneyValue MoneyValue, float Amount ){
		int AmountToRemove = GetMapLinkConfig().GetCurrency(ID).GetAmount(MoneyValue, Amount);
		int LastIndex = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Count() - 1;
		int SmallestCurrency = GetMapLinkConfig().GetCurrency(ID).MoneyValues.Get(LastIndex).Value;
		if (AmountToRemove > 0){
			array<EntityAI> itemsArray = new array<EntityAI>;
			this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
			for (int i = 0; i < itemsArray.Count(); i++){
				ItemBase item = ItemBase.Cast(itemsArray.Get(i));
				if (item && MLCanAccept(ID, item)){
					string ItemType = item.GetType();
					ItemType.ToLower();
					string MoneyType = MoneyValue.Item;
					MoneyType.ToLower();
					if (ItemType == MoneyType){
						int CurQuantity = item.GetQuantity();
						int AmountRemoved = 0;
						if (AmountToRemove < CurQuantity){
							AmountRemoved = MoneyValue.Value * AmountToRemove;
							item.MLSetQuantity(CurQuantity - AmountToRemove);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else if (AmountToRemove == CurQuantity){
							AmountRemoved = MoneyValue.Value * AmountToRemove;
							GetGame().ObjectDelete(item);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else {
							AmountRemoved = MoneyValue.Value * CurQuantity;
							AmountToRemove = AmountToRemove - CurQuantity;
							GetGame().ObjectDelete(item);
							Amount = Amount - AmountRemoved;
						}
						if (AmountToRemove <= 0){
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount;
						}
					}
				}
			}
		}
		this.UpdateInventoryMenu(); // RPC-Call needed?
		return Amount;
	}
	
	//Return How many Items it faild to create in the Inventory
	int MLCreateMoneyInventory(string itemType, int amount)
	{
		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		string itemTypeLower = itemType;
		itemTypeLower.ToLower();
		ItemBase item;
		Ammunition_Base ammoItem;
		int currentAmount = amount;
		bool hasQuantity = ((MLMaxQuantity(itemType) > 0) || MLHasQuantity(itemType));
		if (hasQuantity){
			for (int i = 0; i < itemsArray.Count(); i++){
				if (currentAmount <= 0){
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return 0;
				}
				Class.CastTo(item, itemsArray.Get(i));
				string itemPlayerType = "";
				if (item){
					if (item.IsRuined()){
						continue;
					}
					itemPlayerType = item.GetType();
					itemPlayerType.ToLower();
					if (itemTypeLower == itemPlayerType && !item.IsFullQuantity() && !item.IsMagazine()){
						currentAmount = item.MLAddQuantity(currentAmount);
					}
				}

				Class.CastTo(ammoItem, itemsArray.Get(i));
				if (ammoItem){
					if (ammoItem.IsRuined()){	
						continue;
					}
					itemPlayerType = ammoItem.GetType();
					itemPlayerType.ToLower();
					if (itemTypeLower == itemPlayerType && ammoItem.IsAmmoPile()){
						currentAmount = ammoItem.MLAddQuantity(currentAmount);
					}
				}
			}
		}
		bool stoploop = false;
		int MaxLoop = 5000;
		//any leftover or new stacks
		while (currentAmount > 0 && !stoploop && MaxLoop > 0){
			MaxLoop--;
			ItemBase newItem = ItemBase.Cast(this.GetInventory().CreateInInventory(itemType));
			if (!newItem){
				stoploop = true; //To stop the loop from running away since it couldn't create an item
				for (int j = 0; j < itemsArray.Count(); j++){
					Class.CastTo(item, itemsArray.Get(j));
					if (item){ 
						newItem = ItemBase.Cast(item.GetInventory().CreateInInventory(itemType)); //CreateEntityInCargo	
						if (newItem){
							//Print("[MapLink] NewItem Created " + newItem.GetType() + " in " + item.GetType());
							stoploop = false; //Item was created so we don't need to stop the loop anymore
							break;
						}
					}
				}
			}
			
			Magazine newMagItem = Magazine.Cast(newItem);
			Ammunition_Base newammoItem = Ammunition_Base.Cast(newItem);
			if (newMagItem && !newammoItem)	{	
				int SetAmount = currentAmount;
				if (newMagItem.GetQuantityMax() <= currentAmount){
					SetAmount = currentAmount;
					currentAmount = 0;
				} else {
					SetAmount = newMagItem.GetQuantityMax();
					currentAmount = currentAmount - SetAmount;
				}
				newMagItem.ServerSetAmmoCount(SetAmount);
			} else if (hasQuantity){
				if (newammoItem){
					currentAmount = newammoItem.MLSetQuantity(currentAmount);
	
				}	
				ItemBase newItemBase;
				if (Class.CastTo(newItemBase, newItem)){
					currentAmount = newItemBase.MLSetQuantity(currentAmount);
				}
			} else { //It created just one of the item
				currentAmount--;
			}
		}
		return currentAmount;
	}
	
	void MLCreateMoneyGround(string Type, int Amount){
		int AmountToSpawn = Amount;
		bool HasQuantity = ((MLMaxQuantity(Type) > 0) || MLHasQuantity(Type));
		int MaxQuanity = MLMaxQuantity(Type);
		int StacksRequired = AmountToSpawn;
		if (MaxQuanity != 0){
			StacksRequired = Math.Ceil( AmountToSpawn /  MaxQuanity);
		}
		for (int i = 0; i <= StacksRequired; i++){
			if (AmountToSpawn > 0){
				ItemBase newItem = ItemBase.Cast(GetGame().CreateObjectEx(Type, GetPosition(), ECE_PLACE_ON_SURFACE));
				if (newItem && HasQuantity){
					AmountToSpawn = newItem.MLSetQuantity(AmountToSpawn);
				}
			}
		}
	}
	
	int MLCurrentQuantity(ItemBase money){
		ItemBase moneyItem = ItemBase.Cast(money);
		if (!moneyItem){
			return false;
		}	
		if (MLMaxQuantity(moneyItem.GetType()) == 0){
			return 1;
		}
		if ( moneyItem.IsMagazine() ){
			Magazine mag = Magazine.Cast(moneyItem);
			if (mag){
				return mag.GetAmmoCount();
			}
		}
		return moneyItem.GetQuantity();
	}

	int MLMaxQuantity(string Type)
	{
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + Type + " count" ) ){
			return GetGame().ConfigGetInt(  CFG_MAGAZINESPATH  + " " + Type + " count" );
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + Type + " varQuantityMax" ) ){
			return GetGame().ConfigGetInt( CFG_VEHICLESPATH + " " + Type + " varQuantityMax" ) );
		}
		return 0;
	}
	
	bool MLSetMoneyAmount(ItemBase item, int amount)
	{
		ItemBase money = ItemBase.Cast(item);
		if (!money){
			return false;
		}
		if ( money.IsMagazine() ){
			Magazine mag = Magazine.Cast(money);
			if (mag){
				return true;
				mag.ServerSetAmmoCount(amount);
			}
		}
		else{
			money.SetQuantity(amount);
			return true;
		}
		return false;
	}
	
	bool MLHasQuantity(string Type)
	{   
		
		string path = CFG_MAGAZINESPATH  + " " + Type + " count";
	    if (GetGame().ConfigIsExisting(path)){
	     	if (GetGame().ConfigGetInt(path) > 0){
				return true;
			}
		}
	    path = CFG_VEHICLESPATH  + " " + Type + " quantityBar";
	    if (GetGame().ConfigIsExisting(path))   {
	        return GetGame().ConfigGetInt(path) == 1;
		}
	
	    return false;
	}
	
	
	
}

modded class DayZPlayerMeleeFightLogic_LightHeavy
{
    override bool HandleFightLogic(int pCurrentCommandID, HumanInputController pInputs, EntityAI pEntityInHands, HumanMovementState pMovementState, out bool pContinueAttack)
	{
        PlayerBase player = PlayerBase.Cast(m_DZPlayer);
        
        if (player)
        {
            if (player.IsUnderMapLinkProtection())
            return false;
        }

        return super.HandleFightLogic(pCurrentCommandID, pInputs, pEntityInHands, pMovementState, pContinueAttack);
    }
}

modded class ActionUnpin extends ActionSingleUseBase
{
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
        if (player.IsUnderMapLinkProtection())
            	return false;

		return super.ActionCondition(player, target, item);
	}
}