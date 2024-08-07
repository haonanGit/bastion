
#include "common.h"
#include "quickfix/FixValues.h"
#include "quickfix/Message.h"
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

/*----------------------------------------------dictionary----------------------------------------------------------*/

std::unordered_map<std::string, std::string> msg_type_dict = {
    {"0", "MsgType_Heartbeat"},
    {"1", "MsgType_TestRequest"},
    {"2", "MsgType_ResendRequest"},
    {"3", "MsgType_Reject"},
    {"4", "MsgType_SequenceReset"},
    {"5", "MsgType_Logout"},
    {"A", "MsgType_Logon"},
    {"n", "MsgType_XMLnonFIX"},
    {"6", "MsgType_IOI"},
    {"7", "MsgType_Advertisement"},
    {"8", "MsgType_ExecutionReport"},
    {"9", "MsgType_OrderCancelReject"},
    {"B", "MsgType_News"},
    {"C", "MsgType_Email"},
    {"D", "MsgType_NewOrderSingle"},
    {"E", "MsgType_NewOrderList"},
    {"F", "MsgType_OrderCancelRequest"},
    {"G", "MsgType_OrderCancelReplaceRequest"},
    {"H", "MsgType_OrderStatusRequest"},
    {"J", "MsgType_Allocation"},
    {"K", "MsgType_ListCancelRequest"},
    {"L", "MsgType_ListExecute"},
    {"M", "MsgType_ListStatusRequest"},
    {"N", "MsgType_ListStatus"},
    {"P", "MsgType_AllocationInstructionAck"},
    {"Q", "MsgType_DontKnowTrade"},
    {"R", "MsgType_QuoteRequest"},
    {"S", "MsgType_Quote"},
    {"T", "MsgType_SettlementInstructions"},
    {"V", "MsgType_MarketDataRequest"},
    {"W", "MsgType_MarketDataSnapshotFullRefresh"},
    {"X", "MsgType_MarketDataIncrementalRefresh"},
    {"Y", "MsgType_MarketDataRequestReject"},
    {"Z", "MsgType_QuoteCancel"},
    {"a", "MsgType_QuoteStatusRequest"},
    {"b", "MsgType_QuoteAcknowledgement"},
    {"c", "MsgType_SecurityDefinitionRequest"},
    {"d", "MsgType_SecurityDefinition"},
    {"e", "MsgType_SecurityStatusRequest"},
    {"f", "MsgType_SecurityStatus"},
    {"g", "MsgType_TradingSessionStatusRequest"},
    {"h", "MsgType_TradingSessionStatus"},
    {"i", "MsgType_MassQuote"},
    {"j", "MsgType_BusinessMessageReject"},
    {"k", "MsgType_BidRequest"},
    {"l", "MsgType_BidResponse"},
    {"m", "MsgType_ListStrikePrice"},
    {"P", "MsgType_AllocationAck"},
    {"b", "MsgType_MassQuoteAcknowledgement"},
    {"o", "MsgType_RegistrationInstructions"},
    {"p", "MsgType_RegistrationInstructionsResponse"},
    {"q", "MsgType_OrderMassCancelRequest"},
    {"r", "MsgType_OrderMassCancelReport"},
    {"s", "MsgType_NewOrderCross"},
    {"t", "MsgType_CrossOrderCancelReplaceRequest"},
    {"u", "MsgType_CrossOrderCancelRequest"},
    {"v", "MsgType_SecurityTypeRequest"},
    {"w", "MsgType_SecurityTypes"},
    {"x", "MsgType_SecurityListRequest"},
    {"y", "MsgType_SecurityList"},
    {"z", "MsgType_DerivativeSecurityListRequest"},
    {"AA", "MsgType_DerivativeSecurityList"},
    {"AB", "MsgType_NewOrderMultileg"},
    {"AC", "MsgType_MultilegOrderCancelReplaceRequest"},
    {"AD", "MsgType_TradeCaptureReportRequest"},
    {"AE", "MsgType_TradeCaptureReport"},
    {"AF", "MsgType_OrderMassStatusRequest"},
    {"AG", "MsgType_QuoteRequestReject"},
    {"AH", "MsgType_RFQRequest"},
    {"AI", "MsgType_QuoteStatusReport"},
    {"J", "MsgType_AllocationInstruction"},
    {"AC", "MsgType_MultilegOrderCancelReplace"},
    {"AJ", "MsgType_QuoteResponse"},
    {"AK", "MsgType_Confirmation"},
    {"AL", "MsgType_PositionMaintenanceRequest"},
    {"AM", "MsgType_PositionMaintenanceReport"},
    {"AN", "MsgType_RequestForPositions"},
    {"AO", "MsgType_RequestForPositionsAck"},
    {"AP", "MsgType_PositionReport"},
    {"AQ", "MsgType_TradeCaptureReportRequestAck"},
    {"AR", "MsgType_TradeCaptureReportAck"},
    {"AS", "MsgType_AllocationReport"},
    {"AT", "MsgType_AllocationReportAck"},
    {"AU", "MsgType_ConfirmationAck"},
    {"AV", "MsgType_SettlementInstructionRequest"},
    {"AW", "MsgType_AssignmentReport"},
    {"AX", "MsgType_CollateralRequest"},
    {"AY", "MsgType_CollateralAssignment"},
    {"AZ", "MsgType_CollateralResponse"},
    {"BA", "MsgType_CollateralReport"},
    {"BB", "MsgType_CollateralInquiry"},
    {"BC", "MsgType_NetworkCounterpartySystemStatusRequest"},
    {"BD", "MsgType_NetworkCounterpartySystemStatusResponse"},
    {"BE", "MsgType_UserRequest"},
    {"BF", "MsgType_UserResponse"},
    {"BG", "MsgType_CollateralInquiryAck"},
    {"BH", "MsgType_ConfirmationRequest"},
    {"BO", "MsgType_ContraryIntentionReport"},
    {"BP", "MsgType_SecurityDefinitionUpdateReport"},
    {"BK", "MsgType_SecurityListUpdateReport"},
    {"BL", "MsgType_AdjustedPositionReport"},
    {"BM", "MsgType_AllocationInstructionAlert"},
    {"BN", "MsgType_ExecutionAcknowledgement"},
    {"BJ", "MsgType_TradingSessionList"},
    {"BI", "MsgType_TradingSessionListRequest"},
    {"BQ", "MsgType_SettlementObligationReport"},
    {"BR", "MsgType_DerivativeSecurityListUpdateReport"},
    {"BS", "MsgType_TradingSessionListUpdateReport"},
    {"BT", "MsgType_MarketDefinitionRequest"},
    {"BU", "MsgType_MarketDefinition"},
    {"BV", "MsgType_MarketDefinitionUpdateReport"},
    {"CB", "MsgType_UserNotification"},
    {"BZ", "MsgType_OrderMassActionReport"},
    {"CA", "MsgType_OrderMassActionRequest"},
    {"BW", "MsgType_ApplicationMessageRequest"},
    {"BX", "MsgType_ApplicationMessageRequestAck"},
    {"BY", "MsgType_ApplicationMessageReport"},
    {"b", "MsgType_MassQuoteAck"},
    {"BN", "MsgType_ExecutionAck"},
    {"CC", "MsgType_StreamAssignmentRequest"},
    {"CD", "MsgType_StreamAssignmentReport"},
    {"CE", "MsgType_StreamAssignmentReportACK"},
    {"CH", "MsgType_MarginRequirementInquiry"},
    {"CI", "MsgType_MarginRequirementInquiryAck"},
    {"CJ", "MsgType_MarginRequirementReport"},
    {"CF", "MsgType_PartyDetailsListRequest"},
    {"CG", "MsgType_PartyDetailsListReport"},
    {"CK", "MsgType_PartyDetailsListUpdateReport"},
    {"CL", "MsgType_PartyRiskLimitsRequest"},
    {"CM", "MsgType_PartyRiskLimitsReport"},
    {"CN", "MsgType_SecurityMassStatusRequest"},
    {"CO", "MsgType_SecurityMassStatus"},
    {"CQ", "MsgType_AccountSummaryReport"},
    {"CR", "MsgType_PartyRiskLimitsUpdateReport"},
    {"CS", "MsgType_PartyRiskLimitsDefinitionRequest"},
    {"CT", "MsgType_PartyRiskLimitsDefinitionRequestAck"},
    {"CU", "MsgType_PartyEntitlementsRequest"},
    {"CV", "MsgType_PartyEntitlementsReport"},
    {"CW", "MsgType_QuoteAck"},
    {"CX", "MsgType_PartyDetailsDefinitionRequest"},
    {"CY", "MsgType_PartyDetailsDefinitionRequestAck"},
    {"CZ", "MsgType_PartyEntitlementsUpdateReport"},
    {"DA", "MsgType_PartyEntitlementsDefinitionRequest"},
    {"DB", "MsgType_PartyEntitlementsDefinitionRequestAck"},
    {"DC", "MsgType_TradeMatchReport"},
    {"DD", "MsgType_TradeMatchReportAck"},
    {"DE", "MsgType_PartyRiskLimitsReportAck"},
    {"DF", "MsgType_PartyRiskLimitCheckRequest"},
    {"DG", "MsgType_PartyRiskLimitCheckRequestAck"},
    {"DH", "MsgType_PartyActionRequest"},
    {"DI", "MsgType_PartyActionReport"},
    {"DJ", "MsgType_MassOrder"},
    {"DK", "MsgType_MassOrderAck"},
    {"DL", "MsgType_PositionTransferInstruction"},
    {"DM", "MsgType_PositionTransferInstructionAck"},
    {"DN", "MsgType_PositionTransferReport"},
    {"DO", "MsgType_MarketDataStatisticsRequest"},
    {"DP", "MsgType_MarketDataStatisticsReport"},
    {"DQ", "MsgType_CollateralReportAck"},
    {"DR", "MsgType_MarketDataReport"},
    {"DS", "MsgType_CrossRequest"},
    {"DT", "MsgType_CrossRequestAck"},
    {"DU", "MsgType_AllocationInstructionAlertRequest"},
    {"DV", "MsgType_AllocationInstructionAlertRequestAck"},
    {"DW", "MsgType_TradeAggregationRequest"},
    {"DX", "MsgType_TradeAggregationReport"},
    {"EA", "MsgType_PayManagementReport"},
    {"EB", "MsgType_PayManagementReportAck"},
    {"DY", "MsgType_PayManagementRequest"},
    {"DZ", "MsgType_PayManagementRequestAck"}};

std::unordered_map<std::string, std::string> session_reject_reason_dict = {
    {std::to_string(FIX::SessionRejectReason_INVALID_TAG_NUMBER),
     "Invalid tag number"},
    {std::to_string(FIX::SessionRejectReason_REQUIRED_TAG_MISSING),
     "Required tag missing"},
    {std::to_string(
         FIX::SessionRejectReason_TAG_NOT_DEFINED_FOR_THIS_MESSAGE_TYPE),
     "Tag not defined for this message type"},
    {std::to_string(FIX::SessionRejectReason_UNDEFINED_TAG), "Undefined Tag"},
    {std::to_string(FIX::SessionRejectReason_TAG_SPECIFIED_WITHOUT_A_VALUE),
     "Tag specified without a value"},
    {std::to_string(FIX::SessionRejectReason_VALUE_IS_INCORRECT),
     "Value is incorrect (out of range) for this tag"},
    {std::to_string(FIX::SessionRejectReason_INCORRECT_DATA_FORMAT_FOR_VALUE),
     "Incorrect data format for value"},
    {std::to_string(FIX::SessionRejectReason_DECRYPTION_PROBLEM),
     "Decryption problem"},
    {std::to_string(FIX::SessionRejectReason_SIGNATURE_PROBLEM),
     "Signature problem"},
    {std::to_string(FIX::SessionRejectReason_COMPID_PROBLEM), "CompID problem"},
    {std::to_string(FIX::SessionRejectReason_SENDINGTIME_ACCURACY_PROBLEM),
     "SendingTime accuracy problem"},
    {std::to_string(FIX::SessionRejectReason_INVALID_MSGTYPE),
     "Invalid MsgType"},
    {std::to_string(FIX::SessionRejectReason_XML_VALIDATION_ERROR),
     "XML validation error"},
    {std::to_string(FIX::SessionRejectReason_TAG_APPEARS_MORE_THAN_ONCE),
     "Tag appears more than once"},
    {std::to_string(
         FIX::SessionRejectReason_TAG_SPECIFIED_OUT_OF_REQUIRED_ORDER),
     "Tag specified out of required order"},
    {std::to_string(
         FIX::SessionRejectReason_REPEATING_GROUP_FIELDS_OUT_OF_ORDER),
     "Repeating group fields out of order"},
    {std::to_string(
         FIX::
             SessionRejectReason_INCORRECT_NUMINGROUP_COUNT_FOR_REPEATING_GROUP),
     "Incorrect NumInGroup count for repeating group"},
    {std::to_string(
         FIX::SessionRejectReason_NON_DATA_VALUE_INCLUDES_FIELD_DELIMITER),
     "Non-data value includes field delimiter"},
    {std::to_string(
         FIX::SessionRejectReason_INVALID_UNSUPPORTED_APPLICATION_VERSION),
     "Invalid/unsupported application version"},
    {std::to_string(FIX::SessionRejectReason_OTHER), "Other"}};

std::unordered_map<std::string, std::string> trade_request_result_dict = {
    {"0", "Successful"}, {"1", "Invalid type of trade requested"}};
std::unordered_map<std::string, std::string> trade_request_status_dict = {
    {"0", "Accepted"}, {"1", "Rejected"}};
std::unordered_map<std::string, std::string> side_dict = {{"1", "Buy"},
                                                          {"2", "Sell"}};
std::unordered_map<std::string, std::string> md_entry_type_dict = {
    {"0", "Bid"},
    {"1", "Offer"},
    {"2", "Trade"},
    {"3", "Index Value"},
    {"6", "Settlement Price"}};
std::unordered_map<std::string, std::string> order_status_dict = {
    {std::string(1, FIX::OrdStatus_NEW), "New"},
    {std::string(1, FIX::OrdStatus_PARTIALLY_FILLED), "Partially Filled"},
    {std::string(1, FIX::OrdStatus_FILLED), "Filled"},
    {std::string(1, FIX::OrdStatus_DONE_FOR_DAY), "Done for Day"},
    {std::string(1, FIX::OrdStatus_CANCELED), "Canceled"},
    {std::string(1, FIX::OrdStatus_REPLACED), "Replaced"},
    {std::string(1, FIX::OrdStatus_PENDING_CANCEL), "Pending Cancel"},
    {std::string(1, FIX::OrdStatus_STOPPED), "Stopped"},
    {std::string(1, FIX::OrdStatus_REJECTED), "Rejected"},
    {std::string(1, FIX::OrdStatus_SUSPENDED), "Suspended"},
    {std::string(1, FIX::OrdStatus_PENDING_NEW), "Pending New"},
    {std::string(1, FIX::OrdStatus_CALCULATED), "Calculated"},
    {std::string(1, FIX::OrdStatus_EXPIRED), "Expired"},
    {std::string(1, FIX::OrdStatus_ACCEPTED_FOR_BIDDING),
     "Accepted for Bidding"},
    {std::string(1, FIX::OrdStatus_PENDING_REPLACE), "Pending Replace"}};
/*------------------------------------------------raw string
 * result--------------------------------------------------------*/

std::unordered_map<std::string, std::string> raw_str_dict = {
    {"7", "BeginSeqNo"},
    {"8", "BeginString"},
    {"9", "BodyLength"},
    {"10", "CheckSum"},
    {"11", "ClOrdID"},
    {"15", "Currency"},
    {"16", "EndSeqNo"},
    {"18", "ExecInst"},
    {"34", "MsgSeqNum"},
    {"36", "NewSeqNo"},
    {"37", "OrderID"},
    {"38", "OrderQty"},
    {"40", "OrdType"},
    {"44", "Price"},
    {"45", "RefSeqNum"},
    {"49", "SenderCompID"},
    {"52", "SendingTime"},
    {"54", "Side"},
    {"55", "Symbol"},
    {"56", "TargetCompID"},
    {"58", "Text"},
    {"59", "TimeInForce"},
    {"95", "RawDataLength"},
    {"96", "RawData"},
    {"98", "EncryptMethod"},
    {"108", "HeartBtInt"},
    {"112", "TestReqID"},
    {"141", "ResetSeqNumFlag"},
    {"146", "NoRelatedSym"},
    {"198", "SecondaryOrderID"},
    {"231", "ContractMultiplier"},
    {"262", "MDReqID"},
    {"263", "SubscriptionRequestType"},
    {"264", "MarketDepth"},
    {"265", "MDUpdateType"},
    {"267", "NoMDEntryTypes"},
    {"268", "NoMDEntries"},
    {"269", "MDEntryType"},
    {"270", "MDEntryPx"},
    {"271", "MDEntrySize"},
    {"272", "MDEntryDate"},
    {"371", "RefTagID"},
    {"372", "RefMsgType"},
    {"373", "SessionRejectReason"},
    {"553", "Username"},
    {"554", "Password"},
    {"568", "TradeRequestID"},
    {"569", "TradeRequestType"},
    {"571", "TradeRequestResult"},
    {"746", "OpenInterest"},
    {"923", "UserRequestID"},
    {"926", "UserStatus"},
    {"9011", "DeribitSkipBlockTrades"},
    {"9012", "DeribitShowBlockTradeId"},
    {"100001", "DeribitUserEquity"},
    {"100002", "DeribitUserBalance"},
    {"100003", "DeribitUserInitialMargin"},
    {"100004", "DeribitUserMaintenanceMargin"},
    {"100005", "DeribitUnrealizedPl"},
    {"100006", "DeribitRealizedPl"},
    {"100011", "DeribitTotalPl"},
    {"100013", "DeribitMarginBalance"},
    {"100007", "DeribitTradeAmount"},
    {"100008", "DeribitSinceTimestamp"},
    {"100009", "DeribitTradeId"},
    {"100010", "DeribitLabel"},
    {"100087", "TradeVolume24h"},
    {"100090", "MarkPrice"},
    {"100091", "DeribitLiquidation"},
    {"100092", "CurrentFunding"},
    {"100093", "Funding8h"} {"5127", "DeribitConditionTriggerMethod"}

};

/*------------------------------------------------search dict
 * map--------------------------------------------------------*/

std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
    dict_map = {{"35", msg_type_dict},
                {"373", session_reject_reason_dict},
                {"749", trade_request_result_dict},
                {"750", trade_request_status_dict},
                {"54", side_dict},
                {"39", order_status_dict},
                {"269", md_entry_type_dict}};

/*------------------------------------------------print--------------------------------------------------------*/
void printMsg(const FIX44::Message &msg) {
  std::vector<std::pair<std::string, std::string>> keyValuePairs;
  size_t pos = 0;
  size_t endPos = 0;

  const auto &msg_str = msg.toString();
  while ((pos = msg_str.find('=', pos)) != std::string::npos) {
    size_t keyStart = msg_str.rfind('\x01', pos) + 1;
    std::string key = msg_str.substr(keyStart, pos - keyStart);
    endPos = msg_str.find('\x01', pos);
    std::string value = msg_str.substr(pos + 1, endPos - pos - 1);
    keyValuePairs.emplace_back(make_pair(key, value));
    pos = endPos + 1;
  }
  std::stringstream ss;
  for (const auto &e : keyValuePairs) {
    const auto &key = e.first;
    const auto &val = e.second;
    if (dict_map.count(key) != 0) {
      if (dict_map[key][val].empty()) {
        ss << val << "(T_" << key << ")"
           << ": "
           << "unknow val:[" << val << "]"
           << ", ";
        continue;
      }
      ss << val << "(T_" << key << ")"
         << ":[" << dict_map[key][val] << "]"
         << ", ";
    } else if (raw_str_dict.count(key) != 0) {
      ss << raw_str_dict[key] << "(T_" << key << ")"
         << ":[" << val << "]"
         << ", ";
    } else {
      ss << "(unknown!!!)" << key << ": value:[" << val << "]"
         << ", ";
    }
  }
  ss << std::endl;
  std::cout << ss.str() << std::endl;
}