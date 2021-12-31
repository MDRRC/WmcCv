/***********************************************************************************************************************
   @file   wc_cv.cpp
   @brief  Main application of WifiManualControl (WMC).
 **********************************************************************************************************************/

/***********************************************************************************************************************
   I N C L U D E S
 **********************************************************************************************************************/
#include "wmc_cv.h"
#include "fsmlist.hpp"

/***********************************************************************************************************************
   D E F I N E S
 **********************************************************************************************************************/

/***********************************************************************************************************************
   F O R W A R D  D E C L A R A T I O N S
 **********************************************************************************************************************/
class Idle;
class EnterPomAddress;
class EnterCvNumber;
class EnterCvValueRead;
class EnterCvValueChange;
class EnterCvWrite;

/***********************************************************************************************************************
   D A T A   D E C L A R A T I O N S (exported, local)
 **********************************************************************************************************************/

/* Init variables. */
WmcTft wmcCv::m_wmcCvTft;
uint16_t wmcCv::m_cvNumber    = CV_DEFAULT_NUMBER;
uint16_t wmcCv::m_cvValue     = CV_DEFAULT_VALUE;
uint16_t wmcCv::m_PomAddress  = POM_DEFAULT_ADDRESS;
uint8_t wmcCv::m_timeOutCount = 0;
bool wmcCv::m_PomActive       = false;

/***********************************************************************************************************************
  F U N C T I O N S
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Show screen and handle entering cv number to be changed.
 */
class Idle : public wmcCv
{
    /**
     */
    void entry() override { m_PomActive = false; };

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
            m_PomActive = false;
            m_cvValue   = CV_DEFAULT_VALUE;
            m_cvNumber  = CV_DEFAULT_NUMBER;
            m_wmcCvTft.UpdateStatus("CV PROGRAMMING", true, WmcTft::color_green);
            transit<EnterCvNumber>();
            break;
        case startPom:
            m_PomActive  = true;
            m_cvValue    = CV_DEFAULT_VALUE;
            m_cvNumber   = CV_DEFAULT_NUMBER;
            m_PomAddress = POM_DEFAULT_ADDRESS;
            m_wmcCvTft.UpdateStatus("POM PROGRAMMING", true, WmcTft::color_green);
            transit<EnterPomAddress>();
            break;
        case cvNack:
        case cvData:
        case update:
        case responseNok:
        case responseBusy:
        case responseReady: break;
        }
    }
};

/***********************************************************************************************************************
 * Enter address of loc where Cv will be changed using POM.
 */
class EnterPomAddress : public wmcCv
{
    /**
     */
    void entry() override { m_wmcCvTft.ShowPomAddress(m_PomAddress, true, WmcTft::color_green); };

    /**
     * Handle forwarded pulse switch events.
     */
    void react(cvpulseSwitchEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Status)
        {
        case turn:
            if (e.EventData.Delta > 0)
            {
                m_PomAddress++;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_PomAddress > POM_DEFAULT_ADDRESS)
                {
                    m_PomAddress--;
                    DataChanged = true;
                }
                else
                {
                    m_PomAddress = POM_MAX_ADDRESS;
                    DataChanged  = true;
                }
            }
            break;
        case pushturn:
            if (e.EventData.Delta > 0)
            {
                m_PomAddress += STEP_10;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_PomAddress > STEP_10)
                {
                    m_PomAddress -= STEP_10;
                    DataChanged = true;
                }
                else
                {
                    if (m_PomAddress > POM_DEFAULT_ADDRESS)
                    {
                        m_PomAddress -= STEP_1;
                        DataChanged = true;
                    }
                    else
                    {
                        m_PomAddress = POM_MAX_ADDRESS;
                        DataChanged  = true;
                    }
                }
            }
            break;
        case pushedShort:
            EventCvProg.Request = cvExit;
            send_event(EventCvProg);
            transit<Idle>();
            break;
        case pushedNormal:
        case pushedlong:
            if (m_PomAddress == POM_DEFAULT_ADDRESS)
            {
                m_wmcCvTft.ShowPomAddress(m_PomAddress, false, WmcTft::color_red);
            }
            else
            {
                transit<EnterCvNumber>();
            }
            break;
        case released: break;
        }

        if (DataChanged == true)
        {
            if (m_PomAddress > POM_MAX_ADDRESS)
            {
                m_PomAddress = POM_DEFAULT_ADDRESS;
            }
            m_wmcCvTft.ShowPomAddress(m_PomAddress, false, WmcTft::color_green);
        }
    }

    /**
     * Handle forwarded push button events to reset or increase the address.
     */
    void react(cvpushButtonEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Button)
        {
        case button_0:
            m_PomAddress += STEP_1;
            DataChanged = true;
            break;
        case button_1:
            m_PomAddress += STEP_10;
            DataChanged = true;
            break;
        case button_2:
            m_PomAddress += STEP_100;
            DataChanged = true;
            break;
        case button_3:
            m_PomAddress += STEP_1000;
            DataChanged = true;
            break;
        case button_4:
            m_PomAddress = POM_DEFAULT_ADDRESS;
            DataChanged  = true;
            break;
        case button_5:
            transit<EnterCvNumber>();
            break;
            break;
        case button_power:
            EventCvProg.Request = cvExit;
            send_event(EventCvProg);
            transit<Idle>();
            break;
        case button_none: break;
        }

        if (DataChanged == true)
        {
            if (m_PomAddress > POM_MAX_ADDRESS)
            {
                m_PomAddress = POM_MAX_ADDRESS;
            }
            m_wmcCvTft.ShowPomAddress(m_PomAddress, false, WmcTft::color_green);
        }
    }

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
        case startPom:
        case cvNack:
        case cvData:
        case update:
        case responseNok:
        case responseBusy:
        case responseReady: break;
        }
    }
};

/***********************************************************************************************************************
 * Show screen and handle entering cv number to be changed.
 */
class EnterCvNumber : public wmcCv
{
    /**
     */
    void entry() override { m_wmcCvTft.ShowDccNumber(m_cvNumber, true, m_PomActive); };

    /**
     * Handle forwarded pulse switch events.
     */
    void react(cvpulseSwitchEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Status)
        {
        case turn:
            if (e.EventData.Delta > 0)
            {
                m_cvNumber++;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_cvNumber > CV_DEFAULT_NUMBER)
                {
                    m_cvNumber--;
                    DataChanged = true;
                }
                else
                {
                    if (m_PomActive == true)
                    {
                        m_cvNumber = CV_MAX_NUMBER;
                    }
                    else
                    {
                        m_cvNumber = CV_MAX_NUMBER_CV_MODE;
                    }
                    DataChanged = true;
                }
            }
            break;
        case pushturn:
            if (e.EventData.Delta > 0)
            {
                m_cvNumber += STEP_10;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_cvNumber > STEP_10)
                {
                    m_cvNumber -= STEP_10;
                    DataChanged = true;
                }
                else
                {
                    if (m_cvNumber > CV_DEFAULT_NUMBER)
                    {
                        m_cvNumber -= STEP_1;
                        DataChanged = true;
                    }
                    else
                    {
                        if (m_PomActive == true)
                        {
                            m_cvNumber = CV_MAX_NUMBER;
                        }
                        else
                        {
                            m_cvNumber = CV_MAX_NUMBER_CV_MODE;
                        }
                        DataChanged = true;
                    }
                }
            }
            break;
        case pushedShort:
            if (m_PomActive == true)
            {
                /* Back to entering cv number. */
                m_wmcCvTft.ShowDccNumberRemove(m_PomActive);
                transit<EnterPomAddress>();
            }
            break;
        case pushedNormal:
            if (m_PomActive == false)
            {
                transit<EnterCvValueRead>();
            }
            else
            {
                transit<EnterCvValueChange>();
            }
            break;
        case pushedlong:
            EventCvProg.Request = cvExit;
            send_event(EventCvProg);
            transit<Idle>();
            break;
        case released: break;
        }

        if (DataChanged == true)
        {
            if (m_PomActive == true)
            {
                if (m_cvNumber > CV_MAX_NUMBER) m_cvNumber = CV_DEFAULT_NUMBER;
            }
            else if (m_cvNumber > CV_MAX_NUMBER_CV_MODE)
            {
                m_cvNumber = CV_DEFAULT_NUMBER;
            }
            m_wmcCvTft.ShowDccNumber(m_cvNumber, false, m_PomActive);
        }
    }

    /**
     * Handle forwarded push button events.
     */
    void react(cvpushButtonEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Button)
        {
        case button_0:
            m_cvNumber += STEP_1;
            DataChanged = true;
            break;
        case button_1:
            m_cvNumber += STEP_10;
            DataChanged = true;
            break;
        case button_2:
            m_cvNumber += STEP_100;
            DataChanged = true;
            break;
        case button_3:
            m_cvNumber += STEP_1000;
            DataChanged = true;
            break;
        case button_4:
            m_cvNumber  = CV_DEFAULT_NUMBER;
            DataChanged = true;
            break;
        case button_5:
            if (m_PomActive == false)
            {
                transit<EnterCvValueRead>();
            }
            else
            {
                transit<EnterCvValueChange>();
            }
            break;
        case button_power:
            EventCvProg.Request = cvExit;
            send_event(EventCvProg);
            transit<Idle>();
            break;
        case button_none: break;
        }

        if (DataChanged == true)
        {
            if (m_PomActive == true)
            {
                if (m_cvNumber > CV_MAX_NUMBER) m_cvNumber = CV_DEFAULT_NUMBER;
            }
            else if (m_cvNumber > CV_MAX_NUMBER_CV_MODE)
            {
                m_cvNumber = CV_DEFAULT_NUMBER;
            }
            m_wmcCvTft.ShowDccNumber(m_cvNumber, false, m_PomActive);
        }
    }

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
        case startPom:
        case cvNack:
        case cvData:
        case update:
        case responseNok:
        case responseBusy:
        case responseReady: break;
        }
    }
};

/***********************************************************************************************************************
 * Handle reading cv value
 */
class EnterCvValueRead : public wmcCv
{
    /**
     * Init modules and start connection to wifi network.
     */
    void entry() override
    {
        m_wmcCvTft.UpdateStatus("READING CV", true, WmcTft::color_green);
        EventCvProg.Request  = cvRead;
        EventCvProg.CvNumber = m_cvNumber;
        send_event(EventCvProg);

        m_timeOutCount = 0;
        m_wmcCvTft.UpdateRunningWheel(m_timeOutCount);
    };

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
        case startPom: break;
        case cvNack: transit<EnterCvValueChange>(); break;
        case cvData:
            m_cvValue = e.cvValue;
            transit<EnterCvValueChange>();
            break;
        case update:
            m_timeOutCount++;
            m_wmcCvTft.UpdateRunningWheel(m_timeOutCount);

            EventCvProg.Request = cvStatusRequest;
            send_event(EventCvProg);

            /* If after 20 seconds still no response continue.... */
            if (m_timeOutCount > TIME_OUT_20_SEC)
            {
                transit<EnterCvValueChange>();
            }
            break;
        case responseBusy: break;
        case responseNok: transit<EnterCvValueChange>(); break;
        case responseReady:
            m_cvValue = e.cvValue;
            transit<EnterCvValueChange>();
            break;
            break;
        }
    }
};

/***********************************************************************************************************************
 * Handle changing the cv value.
 */
class EnterCvValueChange : public wmcCv
{
    /**
     */
    void entry() override
    {
        if (m_PomActive == false)
        {
            m_wmcCvTft.UpdateStatus("CV PROGRAMMING", true, WmcTft::color_green);
        }
        m_wmcCvTft.ShowDccValue(m_cvValue, true, m_PomActive);
    };

    /**
     * Handle forwarded pulse switch events.
     */
    void react(cvpulseSwitchEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Status)
        {
        case turn:
            if (e.EventData.Delta > 0)
            {
                m_cvValue++;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_cvValue > CV_DEFAULT_VALUE)
                {
                    m_cvValue--;
                    DataChanged = true;
                }
                else
                {
                    m_cvValue   = CV_MAX_VALUE;
                    DataChanged = true;
                }
            }
            break;
        case pushturn:
            if (e.EventData.Delta > 0)
            {
                m_cvValue += STEP_10;
                DataChanged = true;
            }
            else if (e.EventData.Delta < 0)
            {
                if (m_cvValue > STEP_10)
                {
                    m_cvValue -= STEP_10;
                    DataChanged = true;
                }
                else
                {
                    if (m_cvValue > CV_DEFAULT_VALUE)
                    {
                        m_cvValue -= STEP_1;
                        DataChanged = true;
                    }
                    else
                    {
                        m_cvValue   = CV_MAX_VALUE;
                        DataChanged = true;
                    }
                }
            }
            break;
        case pushedShort:
            /* Back to entering cv number. */
            m_wmcCvTft.ShowDccValueRemove(m_PomActive);
            transit<EnterCvNumber>();
            break;
        case pushedNormal:
        case pushedlong: transit<EnterCvWrite>(); break;
        case released: break;
        }

        if (DataChanged == true)
        {
            if (m_cvValue > CV_MAX_VALUE)
            {
                m_cvValue = CV_DEFAULT_VALUE;
            }
            m_wmcCvTft.ShowDccValue(m_cvValue, false, m_PomActive);
        }
    }

    /**
     * Handle forwarded push button events.
     */
    void react(cvpushButtonEvent const& e)
    {
        bool DataChanged = false;

        switch (e.EventData.Button)
        {
        case button_0:
            m_cvValue += STEP_1;
            DataChanged = true;
            break;
        case button_1:
            m_cvValue += STEP_10;
            DataChanged = true;
            break;
        case button_2:
            m_cvValue += STEP_100;
            DataChanged = true;
            break;
        case button_3: break;
        case button_4:
            m_cvValue   = STEP_1;
            DataChanged = true;
            break;
        case button_5:
            transit<EnterCvWrite>();
            break;
            break;
        case button_power:
            EventCvProg.Request = cvExit;
            send_event(EventCvProg);
            transit<Idle>();
            break;
        case button_none: break;
        }

        if (DataChanged == true)
        {
            if (m_cvValue > CV_MAX_VALUE)
            {
                m_cvValue = CV_DEFAULT_VALUE;
            }
            m_wmcCvTft.ShowDccValue(m_cvValue, false, m_PomActive);
        }
    }

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
        case startPom:
        case cvNack:
        case cvData:
        case update:
        case responseBusy:
        case responseNok:
        case responseReady: break;
        }
    }
};

/***********************************************************************************************************************
 * Write Cv value and wait for result of cv programming.
 */
class EnterCvWrite : public wmcCv
{
    /**
     * Init modules and start connection to wifi network.
     */
    void entry() override
    {
        if (m_PomActive == false)
        {
            EventCvProg.Request = cvWrite;
            m_wmcCvTft.UpdateStatus("WRITING CV", true, WmcTft::color_green);
        }
        else
        {
            EventCvProg.Request = pomWrite;
        }

        /* Fill the CV data, */
        EventCvProg.Address  = m_PomAddress;
        EventCvProg.CvNumber = m_cvNumber;
        EventCvProg.CvValue  = m_cvValue;

        if (m_PomActive == false)
        {
            /* Wait for response when CV programming. */
            m_timeOutCount = 0;
            m_wmcCvTft.UpdateRunningWheel(m_timeOutCount);
        }
        else
        {
            /* No response from Z21 when POM programming, so back to entering address.
             */
            m_wmcCvTft.ShowDccValueRemove(m_PomActive);
            m_wmcCvTft.ShowDccNumberRemove(m_PomActive);
            transit<EnterPomAddress>();
        }

        send_event(EventCvProg);
    }

    /**
     * Handle cv command events.
     */
    void react(cvEvent const& e) override
    {
        switch (e.EventData)
        {
        case startCv:
        case startPom:
        case responseBusy: break;
        case cvData:
        case cvNack:
        case responseNok:
        case responseReady:
            /* Programming ok, back to entering cv number for next CV. */
            if (m_PomActive == false)
            {
                m_wmcCvTft.ShowDccValueRemove(m_PomActive);
                m_wmcCvTft.UpdateStatus("CV PROGRAMMING", true, WmcTft::color_green);
                transit<EnterCvNumber>();
            }
            break;
        case update:
            m_timeOutCount++;
            m_wmcCvTft.UpdateRunningWheel(m_timeOutCount);

            /* If after 10 seconds still no response, keep screen to retry
             * writing.... */
            if (m_timeOutCount > TIME_OUT_10_SEC)
            {
                transit<EnterCvValueChange>();
            }
            break;
        }
    }
};

/***********************************************************************************************************************
 * Default event handlers when not declared in states itself.
 */

void wmcCv::react(cvpulseSwitchEvent const&){};
void wmcCv::react(cvpushButtonEvent const&){};
void wmcCv::react(cvEvent const&){};

/***********************************************************************************************************************
 * Initial state.
 */
FSM_INITIAL_STATE(wmcCv, Idle)
