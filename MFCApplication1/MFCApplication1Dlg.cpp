
// MFCApplication1Dlg.cpp: файл реализации
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"
#include <string>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <ranges>
#include <utility>
#include <cctype>
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

// Реализация
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Диалоговое окно CMFCApplication1Dlg



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_MFCAPPLICATION1_DIALOG, pParent)
	, m_strDecryptedText(_T(""))
	, m_strPlainText(_T(""))
	, m_strCipherText(_T(""))
	, m_strCrackedText(_T(""))
	, m_strFilePath(_T(""))
	, m_nShift(69)
	, m_strEvaluation(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DECRYPT, m_strDecryptedText);
	DDX_Text(pDX, IDC_EDIT_PLAIN, m_strPlainText);
	DDX_Text(pDX, IDC_EDIT_CIPHER, m_strCipherText);
	DDX_Text(pDX, IDC_EDIT_CRACKED, m_strCrackedText);
	DDX_Text(pDX, IDC_EDIT_FILE_PATH, m_strFilePath);
	DDX_Text(pDX, IDC_EDIT_SHIFT, m_nShift);
	DDX_Text(pDX, IDC_STATIC_EVAL, m_strEvaluation);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_OPEN, &CMFCApplication1Dlg::OnBnClickedBtnOpen)
    ON_BN_CLICKED(IDC_BTN_ENCRYPT, &CMFCApplication1Dlg::OnBnClickedBtnEncrypt)
    ON_BN_CLICKED(IDC_BTN_DECRYPT, &CMFCApplication1Dlg::OnBnClickedBtnDecrypt)
    ON_BN_CLICKED(IDC_BTN_CRACK, &CMFCApplication1Dlg::OnBnClickedBtnCrack)
END_MESSAGE_MAP()


// Обработчики сообщений CMFCApplication1Dlg

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Добавление пункта "О программе..." в системное меню.

	// IDM_ABOUTBOX должен быть в пределах системной команды.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Задает значок для этого диалогового окна.  Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);			// Крупный значок
	SetIcon(m_hIcon, FALSE);		// Мелкий значок

	// TODO: добавьте дополнительную инициализацию

	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// При добавлении кнопки свертывания в диалоговое окно нужно воспользоваться приведенным ниже кодом,
//  чтобы нарисовать значок.  Для приложений MFC, использующих модель документов или представлений,
//  это автоматически выполняется рабочей областью.

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Выравнивание значка по центру клиентского прямоугольника
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// Система вызывает эту функцию для получения отображения курсора при перемещении
//  свернутого окна.
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ============================================================================
// Класс шифрования/дешифрования по методу Цезаря
// ============================================================================
class CaesarCipher {
public:
    // Шифрование строки с заданным сдвигом
    static std::string encrypt(const std::string& text, int shift) {
        shift = normalizeShift(shift);
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            result += shiftChar(c, shift);
        }
        return result;
    }

    // Дешифрование строки (обратный сдвиг)
    static std::string decrypt(const std::string& text, int shift) {
        return encrypt(text, -shift);
    }

private:
    // Нормализация сдвига в диапазон [0, 25]
    static constexpr int normalizeShift(int shift) {
        return (shift % 26 + 26) % 26;
    }

    // Сдвиг одного символа с сохранением регистра и пропуском не-букв
    static char shiftChar(char c, int shift) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            char base = std::isupper(static_cast<unsigned char>(c)) ? 'A' : 'a';
            return static_cast<char>(base + (c - base + shift + 26) % 26);
        }
        return c;
    }
};

// ============================================================================
// Класс криптоанализа, взлома и оценки криптостойкости
// ============================================================================
class CaesarCracker {
public:
    struct CrackResult {
        int best_shift{};
        double fitness_score{};
        std::string plaintext;
    };

    // Попытка взлома методом перебора с частотным анализом
    static CrackResult crack(const std::string& ciphertext) {
        std::vector<std::pair<int, double>> candidates;
        candidates.reserve(26);

        for (int shift = 0; shift < 26; ++shift) {
            std::string candidate = CaesarCipher::decrypt(ciphertext, shift);
            double score = calculateFrequencyFitness(candidate);
            candidates.emplace_back(shift, score);
        }

        // C++20 ranges: поиск кандидата с максимальной метрикой соответствия
        auto best_it = std::ranges::max_element(candidates, {},
            [](const auto& p) { return p.second; });

        int best_shift = best_it->first;
        double best_score = best_it->second;
        std::string plaintext = CaesarCipher::decrypt(ciphertext, best_shift);

        return { best_shift, best_score, plaintext };
    }

    // Оценка криптостойкости на основе параметров шифра и результата взлома
    static std::string evaluateStrength(const CrackResult& result, size_t text_len) {
        constexpr double KEY_SPACE_ENTROPY = 4.700439718141092; // ~4.7 бит
        std::string assessment = "Оценка криптостойкости:\n";
        assessment += std::format("  ├─ Пространство ключей: 26 (энтропия: {:.1f} бит)\n", KEY_SPACE_ENTROPY);
        assessment += std::format("  ├─ Метрика соответствия английскому языку: {:.4f}\n", result.fitness_score);
        assessment += "  ├─ Уязвимости: \n";
        assessment += "  │   • Тривиальный полный перебор (brute-force)\n";
        assessment += "  │   • Частотный анализ букв\n";
        assessment += "  │   • Отсутствие диффузии и путаницы (по Шеннону)\n";
        assessment += "  └─ Вывод: ";

        if (text_len < 10) {
            assessment += "КРИПТОСТОЙКОСТЬ НИЗКАЯ (мало данных для анализа, но шифр всё равно небезопасен)\n";
        }
        else {
            assessment += "КРИПТОСТОЙКОСТЬ КРАЙНЕ НИЗКАЯ. Шифр Цезаря не подходит для защиты данных.\n";
        }
        assessment += "  ▶ Рекомендация: использовать AES-256, ChaCha20 или XChaCha20.";
        return assessment;
    }

private:
    // Ожидаемые частоты букв в английском языке (нормализованные)
    static constexpr std::array<double, 26> ENGLISH_FREQ = {
        0.08167, 0.01492, 0.02782, 0.04253, 0.12702, 0.02228, 0.02015,
        0.06094, 0.06966, 0.00153, 0.00772, 0.04025, 0.02406, 0.06749,
        0.07507, 0.01929, 0.00095, 0.05987, 0.06327, 0.09056, 0.02758,
        0.00978, 0.02360, 0.00150, 0.01974, 0.00074
    };

    // Расчёт метрики соответствия текста ожидаемому распределению букв
    static double calculateFrequencyFitness(const std::string& text) {
        std::map<char, int> freqMap;
        int total_letters = 0;

        for (char c : text) {
            if (std::isalpha(static_cast<unsigned char>(c))) {
                freqMap[std::tolower(static_cast<unsigned char>(c))]++;
                total_letters++;
            }
        }
        if (total_letters == 0) return 0.0;

        double score = 0.0;
        // Итерация по STL-контейнеру с использованием structured binding (C++17/20)
        for (const auto& [ch, count] : freqMap) {
            int idx = ch - 'a';
            if (idx >= 0 && idx < 26) {
                double observed = static_cast<double>(count) / total_letters;
                score += observed * ENGLISH_FREQ[idx];
            }
        }
        return score;
    }
};

// ============================================================================
// Утилиты
// ============================================================================
std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Не удалось открыть файл: {}", path.string()));
    }
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return content;
}


// ============================================================================
// Вспомогательные конвертеры CString <-> std::string (Unicode-сборка)
// ============================================================================
inline std::string ToStdStr(const CString& str) {
    // CString -> CStringA -> const char* -> std::string
    return std::string(CStringA(str).GetString());
}

inline CString     ToCStr(const std::string& str) { return CStringW(str.c_str()); }

void CMFCApplication1Dlg::OnBnClickedBtnOpen()
{
	// TODO: добавьте свой код обработчика уведомлений
	CFileDialog dlg(TRUE, _T("txt"), nullptr, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
		_T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"), this);
	if (dlg.DoModal() == IDOK)
	{
		m_strFilePath = dlg.GetPathName();
		try {
			//std::string raw = readFile(std::filesystem::path(CStringA(m_strFilePath)));
            std::filesystem::path fsPath(m_strFilePath.GetString()); // Native wchar_t* для Windows
            std::string raw = readFile(fsPath);
			m_strPlainText = ToCStr(raw);
			m_strCipherText.Empty();
			m_strDecryptedText.Empty();
			m_strCrackedText.Empty();
			m_strEvaluation.Empty();
		}
		catch (const std::exception& e) {
			AfxMessageBox(CString(e.what()), MB_ICONERROR);
		}
		UpdateData(FALSE);
	}
}

void CMFCApplication1Dlg::OnBnClickedBtnEncrypt()
{
    // TODO: добавьте свой код обработчика уведомлений
    UpdateData(TRUE);
    if (m_strPlainText.IsEmpty()) { AfxMessageBox(_T("Сначала загрузите текст!"), MB_ICONWARNING); return; }
    try {
        std::string res = CaesarCipher::encrypt(ToStdStr(m_strPlainText), m_nShift);
        m_strCipherText = ToCStr(res);
        m_strDecryptedText.Empty();
        m_strCrackedText.Empty();
    }
    catch (const std::exception& e) { AfxMessageBox(CString(e.what()), MB_ICONERROR); }
    UpdateData(FALSE);
}

void CMFCApplication1Dlg::OnBnClickedBtnDecrypt()
{
    // TODO: добавьте свой код обработчика уведомлений
    UpdateData(TRUE);
    if (m_strCipherText.IsEmpty()) { AfxMessageBox(_T("Сначала выполните шифрование!"), MB_ICONWARNING); return; }
    try {
        std::string res = CaesarCipher::decrypt(ToStdStr(m_strCipherText), m_nShift);
        m_strDecryptedText = ToCStr(res);
    }
    catch (const std::exception& e) { AfxMessageBox(CString(e.what()), MB_ICONERROR); }
    UpdateData(FALSE);
}

void CMFCApplication1Dlg::OnBnClickedBtnCrack()
{
    // TODO: добавьте свой код обработчика уведомлений
    UpdateData(TRUE);
    if (m_strCipherText.IsEmpty()) { AfxMessageBox(_T("Сначала выполните шифрование!"), MB_ICONWARNING); return; }
    try {
        auto result = CaesarCracker::crack(ToStdStr(m_strCipherText));
        m_strCrackedText = ToCStr(result.plaintext);
        m_strEvaluation = ToCStr(
            CaesarCracker::evaluateStrength(result, ToStdStr(m_strPlainText).length())
        );
    }
    catch (const std::exception& e) { AfxMessageBox(CString(e.what()), MB_ICONERROR); }
    UpdateData(FALSE);
}
