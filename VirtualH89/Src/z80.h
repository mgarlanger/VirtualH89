/// \file z80.h
///
/// \brief Virtual Z-80 processor. Clock accurate.
///
/// \date Mar 7, 2009
///
/// \author Mark Garlanger
///

#ifndef Z80_H_
#define Z80_H_

#include "config.h"
#include "h89Types.h"
#include "cpu.h"
#include "config.h"

#include <csignal>
#include <vector>

#include "AddressBus.h"

class Z80;

typedef void (Z80::*opCodeMethod)(void);
typedef void (Z80::*xd_cbMethod)(BYTE&);


///
/// \brief  Zilog %Z80 %CPU simulator.
///
/// Complete Z80 CPU simulator based on Z80Pack. The original code was converted
/// to C++ and more of the undocumented opcodes have been implemented. Also, fixed
/// a few of the flags for opcodes like DAA.
///
class Z80: public CPU
{
  private:
    std::vector<intrHook *> intrHooks;
    unsigned long checkInter();
    // data

    // Registers

    // Register Pairs (union to allow all type of accesses without casting).
    RP    af, bc, de, hl, wz;
    RP    ix, iy, sp;

    // Reference variables to 'point' to the actual data in the register pair (RP).

    /// Register A
    BYTE&  A;
    /// Register A viewed as a signed byte
    SBYTE& sA;
    /// Register Flags
    BYTE&  F;
    /// Full 16-bit register A and Flags
    WORD&  AF;

    /// Register B
    BYTE&  B;
    /// Register C
    BYTE&  C;
    /// Full 16-bit register B and C
    WORD&  BC;

    /// Register D
    BYTE&  D;
    /// Register E
    BYTE&  E;
    /// Full 16-bit register D and E
    WORD&  DE;

    /// Register H
    BYTE&  H;
    /// Register L
    BYTE&  L;
    /// Full 16-bit register H and L
    WORD&  HL;
    /// Full 16-bit register H and L accessed as a signed 16-bit value
    SWORD& sHL;

    BYTE&  W;
    SBYTE& sW;
    BYTE&  Z;
    WORD&  WZ;

    BYTE&  IXl;
    BYTE&  IXh;
    WORD&  IX;

    BYTE&  IYl;
    BYTE&  IYh;
    WORD&  IY;

    WORD&  SP;

    /// secondary registers. These can be just words since they are always copied to the
    /// primary registers before they can be accessed.
    WORD  _af, _bc, _de, _hl;

    /// Program Counter
    WORD  PC;

    WORD  xxcb_effectiveAddress;
    BYTE  I;                          // Z80 interrupt register
    bool  IFF0, IFF1, IFF2;           // Use a new flag - IFF0 - to easily handle the one instruction before EI takes effect
    unsigned int   R;                 // Z80 refresh register

    unsigned long int   ClockRate_m;
    unsigned long int   ticksPerSecond_m;
    unsigned long int   ticksPerClock_m;

    bool  INT_Line;
    BYTE  intLevel_m;
    bool  processingIntr;

    volatile sig_atomic_t ticks;

    int lastInstTicks;

    AddressBus *ab_m;

    static const int maxNumInst = 4;
    BYTE curInst[maxNumInst];
    BYTE curInstByte;
    BYTE lastInstByte;
    enum instructionPrefix
    {
        ip_none,
        ip_dd,
        ip_fd
    };
    enum cpuMode
    {
        cm_reset,
        cm_running,
        cm_singleStep,
        cm_halt
    };
    cpuMode mode;

    instructionPrefix prefix;
    bool resetReq;

    BYTE IM;

    static const opCodeMethod op_code[256];
    static const opCodeMethod op_cb[256];
    static const opCodeMethod op_ed[256];
    static const opCodeMethod op_xxcb[32];

  public:
    Z80(int clockRate, int ticksPerSecond);
    virtual ~Z80();

    virtual void registerInter(intrCheck *func, void *data);
    virtual void unregisterInter(intrCheck *func);
    virtual void continueRunning(void);
    virtual void waitState(void);

    virtual void reset(void);

    virtual BYTE execute(WORD numInst = 0);
    virtual BYTE step(void);

    virtual void raiseNMI(void);
    virtual void addClockTicks(void);
    virtual void setAddressBus(AddressBus *ab);
    virtual void setSpeed(bool fast);

    /// \todo - fix this for general case with any instruction on the address bus
    virtual void raiseINT();
    virtual void lowerINT();

    /// \todo - remove these vvvvvvvv
//  virtual void setCPUStates(BYTE error, BYTE state);
    void traceInstructions(void);
    // -- remove ^^^^^

  protected:
    /// Signed Flag
    static const BYTE S_FLAG  = 0x80;
    /// Zero Flag
    static const BYTE Z_FLAG  = 0x40;
    /// Unused Flag 2
    static const BYTE N2_FLAG = 0x20;
    /// Half-carry Flag
    static const BYTE H_FLAG  = 0x10;
    /// Unused Flag 1
    static const BYTE N1_FLAG = 0x08;
    /// Parity Flag
    static const BYTE P_FLAG  = 0x04;
    /// Negative Flag
    static const BYTE N_FLAG  = 0x02;
    /// Carry Flag
    static const BYTE C_FLAG  = 0x01;

#if PARITY_TABLE
    ///
    ///  Table to determine parity flag as fast as possible
    ///
    ///  This table holds the value that the Parity Flag should be set to base on the
    ///  index byte.
    ///
    static const BYTE parity[256];
#endif

#if USE_TABLE
    ///
    /// Table to contains the values for Z, S, and P flags. This is done to reduce
    /// duplicated code and to be slightly faster, than the individual settings of
    ///  the flags.
    ///
    static const BYTE ZSP[256];
#endif

    int cpu_state, int_type;

    // Register related
    inline BYTE& getReg8(BYTE val);

    inline BYTE& getCoreReg8(BYTE val);

    inline BYTE getReg8Val(BYTE val);

    inline BYTE getCoreReg8Val(BYTE val);

    // Register related
    inline WORD& getReg16(BYTE val);

    inline WORD& getCoreReg16(BYTE val);

    inline WORD& getHLReg16(void);

    inline WORD getHLReg16Val(void);

    inline WORD getIxReg16Val(void);

    inline WORD getIndirectAddr(void);

    inline WORD getReg16Val(BYTE val);

    inline WORD getCoreReg16Val(BYTE val);

    inline WORD& getReg16qq(BYTE val);

    inline WORD& getCoreReg16qq(BYTE val);

    inline WORD getReg16qqVal(BYTE val);

    inline WORD getCoreReg16qqVal(BYTE val);

    inline BYTE getBit(BYTE val);

    inline bool checkCondition(BYTE val);

    // Routines related to the stack.

    inline void PUSH(WORD x);
    inline void POP(WORD& x);

    // Routines related to FLAGS.

    inline void SET_FLAGS(BYTE flags);
    inline void CLEAR_FLAGS(BYTE flags);
    inline bool CHECK_FLAGS(BYTE flags);
    inline void COND_FLAGS(bool cond, BYTE flags);

    /// \todo - determine which way to keep.
    inline void SET_ZSP_FLAGS(BYTE val);

    /// \brief read next instruction byte and increment PC, if not processing interrupt
    inline BYTE readInst(void)
    {
        ticks -= 4;
        BYTE val = ab_m->readByte(PC, processingIntr);
        if (!processingIntr) {
            ++PC;
        }
        return val;
    };
    inline BYTE readMEM(WORD addr)
    {
        ticks -= 3;
        return (ab_m->readByte(addr));
    };

    inline void writeMEM(WORD addr, BYTE val)
    {
        ab_m->writeByte(addr, val);
        ticks -= 3;
    };

    inline WORD readWord(WORD addr)
    {
        return ((readMEM(addr + 1) << 8) | readMEM(addr));
    }

    inline void writeWord(WORD addr, WORD value)
    {
        writeMEM(addr++, value & 0xff);
        writeMEM(addr, value >> 8);
    }

    inline BYTE READn(void)
    {
        return (readMEM(PC++));
    };

    inline SBYTE sREADn(void)
    {
        return ((SBYTE) readMEM(PC++));
    };

    inline WORD READnn(void)
    {
        PC += 2;
        return ((readMEM(PC - 1) << 8) | readMEM(PC - 2));
    };

  private:

    /// \todo - move this outside of the z80 class.
    static const unsigned int speedUpFactor_c = 40;

    //-------------------------
    //
    // generic routines (i.e. multiple opcodes call them )
    //
    //  This routines share common code and are inlined to help avoid performance
    // impact.
    //
    inline void op_add_reg16(WORD&, WORD);
    inline void op_and(const BYTE);
    inline void op_or(const BYTE);
    inline void op_xor(const BYTE);
    inline void op_add(const BYTE);
    inline void op_adc(const BYTE);
    inline void op_sub(const BYTE);
    inline void op_sbc(const BYTE);
    inline void op_cp(const BYTE);
    inline void op_inc(BYTE&);
    inline void op_dec(BYTE&);
    inline void op_srl(BYTE&);
    inline void op_sla(BYTE&);
    inline void op_undoc_sll(BYTE&);
    inline void op_rl(BYTE&);
    inline void op_rr(BYTE&);
    inline void op_rrc(BYTE&);
    inline void op_rlc(BYTE&);
    inline void op_sra(BYTE&);
    inline void op_in_ic(BYTE&);

    //-------------------------
    //
    // All one byte opcodes.
    //
    void op_ld_x_x(void);
    void op_ld_x_n(void);
    void op_ld_xx_nn(void);
    void op_inc_x(void);
    void op_dec_x(void);
    void op_nop(void);
    void op_halt(void);
    void op_scf(void);
    void op_ccf(void);
    void op_cpl(void);
    void op_daa(void);
    void op_ei(void);
    void op_di(void);
    void op_in(void);
    void op_out(void);
    void op_ld_a_ibc(void);
    void op_ld_a_ide(void);
    void op_ld_a_inn(void);
    void op_ld_ibc_a(void);
    void op_ld_ide_a(void);
    void op_ld_inn_a(void);
    void op_ld_ihl_x(void);
    void op_ld_ihl_n(void);
    void op_ld_x_ihl(void);
    void op_ld_sp_hl(void);
    void op_ld_hl_inn(void);
    void op_ld_inn_hl(void);
    void op_inc_xx(void);
    void op_dec_xx(void);
    void op_add_hl_xx(void);
    void op_and_x(void);
    void op_and_ihl(void);
    void op_and_n(void);
    void op_or_x(void);
    void op_or_ihl(void);
    void op_or_n(void);
    void op_xor_x(void);
    void op_xor_ihl(void);
    void op_xor_n(void);
    void op_add_x(void);
    void op_add_ihl(void);
    void op_add_n(void);
    void op_adc_x(void);
    void op_adc_ihl(void);
    void op_adc_n(void);
    void op_sub_x(void);
    void op_sub_ihl(void);
    void op_sub_n(void);
    void op_sbc_x(void);
    void op_sbc_ihl(void);
    void op_sbc_n(void);
    void op_cp_x(void);
    void op_cp_ihl(void);
    void op_cp_n(void);
    void op_inc_ihl(void);
    void op_dec_ihl(void);
    void op_rlc_a(void);
    void op_rrc_a(void);
    void op_rl_a(void);
    void op_rr_a(void);
    void op_ex_de_hl(void);
    void op_ex_af_af(void);
    void op_exx(void);
    void op_ex_isp_hl(void);
    void op_push_xx(void);
    void op_pop_xx(void);

    void op_jp(void);
    void op_jp_hl(void);
    void op_jr(void);
    void op_djnz(void);
    void op_call(void);
    void op_ret(void);
    void op_jp_cc(void);
    void op_call_cc(void);
    void op_ret_cc(void);

    void op_jr_z(void);
    void op_jr_nz(void);
    void op_jr_c(void);
    void op_jr_nc(void);

    void op_rst00(void);
    void op_rst08(void);
    void op_rst10(void);
    void op_rst18(void);
    void op_rst20(void);
    void op_rst28(void);
    void op_rst30(void);
    void op_rst38(void);
#if !(INDIVIDUAL_FUNCTIONS)
    /// \todo - enable arbitrary instructions for interrupts, thus the rstnn() could
    // be removed
    void op_rst(void);
#endif

    // routines to handle prefix instructions.
    void op_cb_handle(void);
    void op_dd_handle(void);
    void op_ed_handle(void);
    void op_fd_handle(void);

    void op_ed_nop(void);

    //
    // Instructions with CB prefix
    //
    void op_srl_x(void);
    void op_srl_ihl(void);
    void op_sla_x(void);
    void op_sla_ihl(void);
    void op_rl_x(void);
    void op_rl_ihl(void);
    void op_rr_x(void);
    void op_rr_ihl(void);
    void op_rrc_x(void);
    void op_rrc_ihl(void);
    void op_rlc_x(void);
    void op_rlc_ihl(void);
    void op_sra_x(void);
    void op_sra_ihl(void);
    void op_sb_n(BYTE& val);
    void op_sb_n_x(void);
    void op_sb_n_ihl(void);
    void op_rb_n(BYTE& val);
    void op_rb_n_x(void);
    void op_rb_n_ihl(void);
    void op_tb_n(BYTE m, BYTE bit);
    void op_tb_n_x(void);
    void op_tb_n_ihl(void);

    //
    // undocumented CB instructions
    //
    void op_sll_x(void);
    void op_sll_ihl(void);

    //
    // ED Prefix instructions
    //
    void op_im0(void);
    void op_im1(void);
    void op_im2(void);
    void op_reti(void);
    void op_retn(void);
    void op_neg(void);
    void op_in_x_c(void);
    void op_out_c_x(void);
    void op_in_f_ic(void);
    void op_out_c_0(void);
    void op_ini(void);
    void op_inir(void);
    void op_ind(void);
    void op_indr(void);
    void op_outi(void);
    void op_otir(void);
    void op_outd(void);
    void op_otdr(void);
    void op_ld_a_i(void);
    void op_ld_a_r(void);
    void op_ld_i_a(void);
    void op_ld_r_a(void);
    void op_ld_xx_inn(void);
    void op_ld_inn_xx(void);
    void op_adc16(WORD op);
    void op_sbc16(WORD op);
    void op_adc_hl_xx(void);
    void op_sbc_hl_xx(void);
    void op_ldi(void);
    void op_ldir(void);
    void op_ldd(void);
    void op_lddr(void);
    void op_cpi(void);
    void op_cpir(void);
    void op_cpd(void);
    void op_cpdr(void);
    void op_rld_ihl(void);
    void op_rrd_ihl(void);

    //
    // DD CB prefix instructions
    // FD CB prefix instructions
    //
    void op_tb_n_xx_d(void);
    void op_rb_n_xx_d(void);
    void op_sb_n_xx_d(void);
    void op_rlc_xx_d(void);
    void op_rrc_xx_d(void);
    void op_rl_xx_d(void);
    void op_rr_xx_d(void);
    void op_sla_xx_d(void);
    void op_sll_xx_d(void);
    void op_sra_xx_d(void);
    void op_srl_xx_d(void);

    void op_xxx_xx_d(xd_cbMethod operation);

};

// operation of simulated CPU

enum
{
    STOPPED_C     = 0,
    RUN_C         = 1,
    HALT_C        = 2,
    SINGLE_STEP_C = 3,
    POWEROFF_C    = 255
};

// Types of interrupts
const int Intr_NMI = 0x01;
const int Intr_INT = 0x02;

#endif // Z80_H_
