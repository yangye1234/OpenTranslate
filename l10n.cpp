#include "l10n.h"

namespace {
QString en(const QString &key)
{
    if (key == "dialog.title") return "OpenTranslate";
    if (key == "dialog.original.placeholder") return "Enter source text and press Enter";
    if (key == "dialog.result.placeholder") return "Translation result";
    if (key == "dialog.language.auto") return "Auto";
    if (key == "dialog.error.invalid_pair") return "Invalid language pair.";
    if (key == "dialog.status.translating") return "Translating...";
    if (key == "dialog.tooltip.swap") return "Swap language direction";
    if (key == "dialog.tooltip.play_audio") return "Play translation audio";
    if (key == "dialog.tooltip.stop_audio") return "Stop audio";
    if (key == "dialog.tooltip.pin") return "Pin on top";
    if (key == "dialog.tooltip.settings") return "Open settings";
    if (key == "dialog.error.no_selection") return "No selected text found.";
    if (key == "dialog.hotkey.fallback.swap") return "Swap hotkey failed, reverted to default.";
    if (key == "dialog.hotkey.fallback.pin") return "On-top hotkey failed, reverted to default.";
    if (key == "dialog.hotkey.fallback.settings") return "Settings hotkey failed, reverted to default.";
    if (key == "dialog.hotkey.fallback.selection") return "Translate selection hotkey failed, reverted to default.";
    if (key == "dialog.hotkey.failed.swap") return "Swap hotkey registration failed.";
    if (key == "dialog.hotkey.failed.pin") return "On-top hotkey registration failed.";
    if (key == "dialog.hotkey.failed.settings") return "Settings hotkey registration failed.";
    if (key == "dialog.hotkey.failed.selection") return "Translate selection hotkey registration failed.";

    if (key == "settings.title") return "Settings";
    if (key == "settings.group.app") return "Application";
    if (key == "settings.group.shortcuts") return "Shortcuts";
    if (key == "settings.label.app_language") return "App Language";
    if (key == "settings.label.provider") return "Default Provider";
    if (key == "settings.label.pairs_default") return "Language Pairs / Default";
    if (key == "settings.shortcuts.swap") return "Swap Language";
    if (key == "settings.shortcuts.pin") return "Toggle On Top";
    if (key == "settings.shortcuts.settings") return "Open Settings";
    if (key == "settings.shortcuts.selection") return "Translate Selection";
    if (key == "settings.group.baidu") return "Baidu API";
    if (key == "settings.baidu.enabled") return "Enable Baidu translator";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "OpenAI-compatible API";
    if (key == "settings.generic.enabled") return "Enable OpenAI-compatible API";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "Model";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "Prompt Template";
    if (key == "settings.group.deepl") return "DeepL API";
    if (key == "settings.deepl.enabled") return "Enable DeepL";
    if (key == "settings.group.dictionary") return "Dictionary";
    if (key == "settings.dictionary.enabled") return "Enable dictionary provider";
    if (key == "settings.group.data") return "Cache and History";
    if (key == "settings.cache.enabled") return "Enable translation cache";
    if (key == "settings.cache.clear") return "Clear Cache";
    if (key == "settings.cache.export") return "Export Cache";
    if (key == "settings.cache.import") return "Import Cache";
    if (key == "settings.cache.clear.title") return "Clear cache";
    if (key == "settings.cache.clear.body") return "Clear all cached translations?";
    if (key == "settings.cache.clear.done") return "Cache cleared.";
    if (key == "settings.cache.clear.failed") return "Could not clear cache.";
    if (key == "settings.cache.export.done") return "Cache exported.";
    if (key == "settings.cache.export.failed") return "Could not export cache.";
    if (key == "settings.cache.import.done") return "Cache imported.";
    if (key == "settings.cache.import.failed") return "Could not import cache.";
    if (key == "settings.history.enabled") return "Enable translation history";
    if (key == "settings.history.max_entries") return "Max history entries";
    if (key == "settings.history.view") return "View History";
    if (key == "settings.history.clear") return "Clear History";
    if (key == "settings.history.empty") return "No translation history yet.";
    if (key == "settings.history.copy_all") return "Copy All";
    if (key == "settings.history.clear.title") return "Clear history";
    if (key == "settings.history.clear.body") return "Clear all translation history?";
    if (key == "settings.history.clear.done") return "History cleared.";
    if (key == "settings.history.clear.failed") return "Could not clear history.";
    if (key == "settings.operation.done") return "Done";
    if (key == "settings.operation.failed") return "Failed";
    if (key == "settings.group.pairs") return "Language Pairs";
    if (key == "settings.pairs.placeholder") return "en->zh";
    if (key == "settings.pairs.add") return "Add";
    if (key == "settings.pairs.edit") return "Edit Selected";
    if (key == "settings.pairs.remove") return "Remove";
    if (key == "settings.save") return "Save";
    if (key == "settings.saved") return "Saved";

    if (key == "settings.error.invalid_pair.title") return "Invalid language pair";
    if (key == "settings.error.invalid_pair.body") return "Use format like en->zh.";
    if (key == "settings.error.duplicate.title") return "Duplicate";
    if (key == "settings.error.duplicate.body") return "This language pair already exists.";
    if (key == "settings.error.shortcut_conflict.title") return "Shortcut conflict";
    if (key == "settings.error.shortcut_conflict.body") return "Shortcuts cannot be duplicated.";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}

QString zhCN(const QString &key)
{
    if (key == "dialog.title") return "开源翻译";
    if (key == "dialog.original.placeholder") return "输入原文后按回车翻译";
    if (key == "dialog.result.placeholder") return "翻译结果";
    if (key == "dialog.language.auto") return "自动选择";
    if (key == "dialog.error.invalid_pair") return "语言方向格式不正确。";
    if (key == "dialog.status.translating") return "翻译中...";
    if (key == "dialog.tooltip.swap") return "切换翻译方向";
    if (key == "dialog.tooltip.play_audio") return "播放译文语音";
    if (key == "dialog.tooltip.stop_audio") return "停止语音";
    if (key == "dialog.tooltip.pin") return "窗口置顶";
    if (key == "dialog.tooltip.settings") return "打开设置";
    if (key == "dialog.error.no_selection") return "未获取到选中文本。";
    if (key == "dialog.hotkey.fallback.swap") return "语言互转快捷键注册失败，已回退默认。";
    if (key == "dialog.hotkey.fallback.pin") return "置顶快捷键注册失败，已回退默认。";
    if (key == "dialog.hotkey.fallback.settings") return "设置快捷键注册失败，已回退默认。";
    if (key == "dialog.hotkey.fallback.selection") return "翻译选中文本快捷键注册失败，已回退默认。";
    if (key == "dialog.hotkey.failed.swap") return "语言互转快捷键注册失败。";
    if (key == "dialog.hotkey.failed.pin") return "置顶快捷键注册失败。";
    if (key == "dialog.hotkey.failed.settings") return "设置快捷键注册失败。";
    if (key == "dialog.hotkey.failed.selection") return "翻译选中文本快捷键注册失败。";

    if (key == "settings.title") return "设置";
    if (key == "settings.group.app") return "应用";
    if (key == "settings.group.shortcuts") return "快捷键";
    if (key == "settings.label.app_language") return "应用语言";
    if (key == "settings.label.provider") return "默认翻译服务";
    if (key == "settings.label.pairs_default") return "语言对 / 默认";
    if (key == "settings.shortcuts.swap") return "语言互转";
    if (key == "settings.shortcuts.pin") return "切换置顶";
    if (key == "settings.shortcuts.settings") return "打开设置";
    if (key == "settings.shortcuts.selection") return "翻译选中文本";
    if (key == "settings.group.baidu") return "百度翻译 API";
    if (key == "settings.baidu.enabled") return "启用百度翻译";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "OpenAI 兼容 API";
    if (key == "settings.generic.enabled") return "启用 OpenAI 兼容 API";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "模型";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "翻译提示词";
    if (key == "settings.group.deepl") return "DeepL API";
    if (key == "settings.deepl.enabled") return "启用 DeepL";
    if (key == "settings.group.dictionary") return "词典";
    if (key == "settings.dictionary.enabled") return "启用词典服务";
    if (key == "settings.group.data") return "缓存与历史";
    if (key == "settings.cache.enabled") return "启用翻译缓存";
    if (key == "settings.cache.clear") return "清理缓存";
    if (key == "settings.cache.export") return "导出缓存";
    if (key == "settings.cache.import") return "导入缓存";
    if (key == "settings.cache.clear.title") return "清理缓存";
    if (key == "settings.cache.clear.body") return "确定清理所有翻译缓存吗？";
    if (key == "settings.cache.clear.done") return "缓存已清理。";
    if (key == "settings.cache.clear.failed") return "缓存清理失败。";
    if (key == "settings.cache.export.done") return "缓存已导出。";
    if (key == "settings.cache.export.failed") return "缓存导出失败。";
    if (key == "settings.cache.import.done") return "缓存已导入。";
    if (key == "settings.cache.import.failed") return "缓存导入失败。";
    if (key == "settings.history.enabled") return "启用翻译历史";
    if (key == "settings.history.max_entries") return "最大历史条数";
    if (key == "settings.history.view") return "查看历史";
    if (key == "settings.history.clear") return "清空历史";
    if (key == "settings.history.empty") return "暂无翻译历史。";
    if (key == "settings.history.copy_all") return "复制全部";
    if (key == "settings.history.clear.title") return "清空历史";
    if (key == "settings.history.clear.body") return "确定清空所有翻译历史吗？";
    if (key == "settings.history.clear.done") return "历史已清空。";
    if (key == "settings.history.clear.failed") return "历史清空失败。";
    if (key == "settings.operation.done") return "完成";
    if (key == "settings.operation.failed") return "失败";
    if (key == "settings.group.pairs") return "语言方向";
    if (key == "settings.pairs.placeholder") return "例如 en->zh";
    if (key == "settings.pairs.add") return "新增";
    if (key == "settings.pairs.edit") return "编辑当前";
    if (key == "settings.pairs.remove") return "删除";
    if (key == "settings.save") return "保存";
    if (key == "settings.saved") return "已保存";

    if (key == "settings.error.invalid_pair.title") return "语言方向格式错误";
    if (key == "settings.error.invalid_pair.body") return "请使用类似 en->zh 的格式。";
    if (key == "settings.error.duplicate.title") return "重复";
    if (key == "settings.error.duplicate.body") return "该语言方向已存在。";
    if (key == "settings.error.shortcut_conflict.title") return "快捷键冲突";
    if (key == "settings.error.shortcut_conflict.body") return "三个动作不能使用相同快捷键。";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}

QString zhTW(const QString &key)
{
    if (key == "dialog.title") return "開源翻譯";
    if (key == "dialog.original.placeholder") return "輸入原文後按 Enter 翻譯";
    if (key == "dialog.result.placeholder") return "翻譯結果";
    if (key == "dialog.language.auto") return "自動選擇";
    if (key == "dialog.error.invalid_pair") return "語言方向格式不正確。";
    if (key == "dialog.status.translating") return "翻譯中...";
    if (key == "dialog.tooltip.swap") return "切換翻譯方向";
    if (key == "dialog.tooltip.play_audio") return "播放譯文語音";
    if (key == "dialog.tooltip.stop_audio") return "停止語音";
    if (key == "dialog.tooltip.pin") return "視窗置頂";
    if (key == "dialog.tooltip.settings") return "開啟設定";
    if (key == "dialog.error.no_selection") return "未取得選取文字。";
    if (key == "dialog.hotkey.fallback.swap") return "語言互轉快捷鍵註冊失敗，已回退預設。";
    if (key == "dialog.hotkey.fallback.pin") return "置頂快捷鍵註冊失敗，已回退預設。";
    if (key == "dialog.hotkey.fallback.settings") return "設定快捷鍵註冊失敗，已回退預設。";
    if (key == "dialog.hotkey.fallback.selection") return "翻譯選取文字快捷鍵註冊失敗，已回退預設。";
    if (key == "dialog.hotkey.failed.swap") return "語言互轉快捷鍵註冊失敗。";
    if (key == "dialog.hotkey.failed.pin") return "置頂快捷鍵註冊失敗。";
    if (key == "dialog.hotkey.failed.settings") return "設定快捷鍵註冊失敗。";
    if (key == "dialog.hotkey.failed.selection") return "翻譯選取文字快捷鍵註冊失敗。";

    if (key == "settings.title") return "設定";
    if (key == "settings.group.app") return "應用";
    if (key == "settings.group.shortcuts") return "快捷鍵";
    if (key == "settings.label.app_language") return "應用語言";
    if (key == "settings.label.provider") return "預設翻譯服務";
    if (key == "settings.label.pairs_default") return "語言對 / 預設";
    if (key == "settings.shortcuts.swap") return "語言互轉";
    if (key == "settings.shortcuts.pin") return "切換置頂";
    if (key == "settings.shortcuts.settings") return "開啟設定";
    if (key == "settings.shortcuts.selection") return "翻譯選取文字";
    if (key == "settings.group.baidu") return "百度翻譯 API";
    if (key == "settings.baidu.enabled") return "啟用百度翻譯";
    if (key == "settings.baidu.app_id") return "App ID";
    if (key == "settings.baidu.app_key") return "App Key";
    if (key == "settings.group.generic") return "OpenAI 相容 API";
    if (key == "settings.generic.enabled") return "啟用 OpenAI 相容 API";
    if (key == "settings.generic.base_url") return "Base URL";
    if (key == "settings.generic.model") return "模型";
    if (key == "settings.generic.api_key") return "API Key";
    if (key == "settings.generic.prompt") return "翻譯提示詞";
    if (key == "settings.group.deepl") return "DeepL API";
    if (key == "settings.deepl.enabled") return "啟用 DeepL";
    if (key == "settings.group.dictionary") return "詞典";
    if (key == "settings.dictionary.enabled") return "啟用詞典服務";
    if (key == "settings.group.data") return "快取與歷史";
    if (key == "settings.cache.enabled") return "啟用翻譯快取";
    if (key == "settings.cache.clear") return "清理快取";
    if (key == "settings.cache.export") return "匯出快取";
    if (key == "settings.cache.import") return "匯入快取";
    if (key == "settings.cache.clear.title") return "清理快取";
    if (key == "settings.cache.clear.body") return "確定清理所有翻譯快取嗎？";
    if (key == "settings.cache.clear.done") return "快取已清理。";
    if (key == "settings.cache.clear.failed") return "快取清理失敗。";
    if (key == "settings.cache.export.done") return "快取已匯出。";
    if (key == "settings.cache.export.failed") return "快取匯出失敗。";
    if (key == "settings.cache.import.done") return "快取已匯入。";
    if (key == "settings.cache.import.failed") return "快取匯入失敗。";
    if (key == "settings.history.enabled") return "啟用翻譯歷史";
    if (key == "settings.history.max_entries") return "最大歷史筆數";
    if (key == "settings.history.view") return "查看歷史";
    if (key == "settings.history.clear") return "清空歷史";
    if (key == "settings.history.empty") return "暫無翻譯歷史。";
    if (key == "settings.history.copy_all") return "複製全部";
    if (key == "settings.history.clear.title") return "清空歷史";
    if (key == "settings.history.clear.body") return "確定清空所有翻譯歷史嗎？";
    if (key == "settings.history.clear.done") return "歷史已清空。";
    if (key == "settings.history.clear.failed") return "歷史清空失敗。";
    if (key == "settings.operation.done") return "完成";
    if (key == "settings.operation.failed") return "失敗";
    if (key == "settings.group.pairs") return "語言方向";
    if (key == "settings.pairs.placeholder") return "例如 en->zh";
    if (key == "settings.pairs.add") return "新增";
    if (key == "settings.pairs.edit") return "編輯目前";
    if (key == "settings.pairs.remove") return "刪除";
    if (key == "settings.save") return "保存";
    if (key == "settings.saved") return "已保存";

    if (key == "settings.error.invalid_pair.title") return "語言方向格式錯誤";
    if (key == "settings.error.invalid_pair.body") return "請使用類似 en->zh 的格式。";
    if (key == "settings.error.duplicate.title") return "重複";
    if (key == "settings.error.duplicate.body") return "該語言方向已存在。";
    if (key == "settings.error.shortcut_conflict.title") return "快捷鍵衝突";
    if (key == "settings.error.shortcut_conflict.body") return "三個動作不能使用相同快捷鍵。";

    if (key == "language.english") return "English";
    if (key == "language.zh_cn") return "简体中文";
    if (key == "language.zh_tw") return "繁體中文";

    return key;
}
}

namespace L10n {

QString text(AppLanguage language, const QString &key)
{
    switch (language) {
    case AppLanguage::English:
        return en(key);
    case AppLanguage::TraditionalChinese:
        return zhTW(key);
    case AppLanguage::SimplifiedChinese:
    default:
        return zhCN(key);
    }
}

} // namespace L10n
