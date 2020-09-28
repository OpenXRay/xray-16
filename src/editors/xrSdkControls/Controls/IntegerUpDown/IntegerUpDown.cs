// Based on original System.Windows.Forms.NumericUpDown
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    /// <summary>
    ///     Represents a Windows up-down control that displays integer values.
    /// </summary>
    [ComVisible(true),
     ClassInterface(ClassInterfaceType.AutoDispatch),
     DefaultProperty("Value"),
     DefaultEvent("ValueChanged"),
     DefaultBindingProperty("Value")]
    public class IntegerUpDown : UpDownBase, ISupportInitialize
    {
        private const bool DefaultHexadecimal = false;
        private const int InvalidValue = -1;
        private static readonly Int32 DefaultValue = 0;
        private static readonly Int32 DefaultMinimum = 0;
        private static readonly Int32 DefaultMaximum = 100;
        private static readonly Int32 DefaultIncrement = 1;

        // Provides for finer acceleration behavior. 
        private IntegerUpDownAccelerationCollection accelerations;

        // the current NumericUpDownAcceleration object. 
        private int accelerationsCurrentIndex;

        // Used to calculate the time elapsed since the up/down button was pressed,
        // to know when to get the next entry in the accelaration table. 
        private long buttonPressedStartTime;
        private Int32 currentValue = DefaultValue;
        private bool currentValueChanged;
        private bool hexadecimal = DefaultHexadecimal;

        /// <summary>
        ///     The amount to increment by.
        /// </summary>
        private Int32 increment = DefaultIncrement;

        private bool initializing;
        private Int32 maximum = DefaultMaximum;
        private Int32 minimum = DefaultMinimum;
        private EventHandler onValueChanged;

        /// <summary>
        ///     [To be supplied.]
        /// </summary>
        [
            SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters") // "0" is the default value for numeric up down.
            // So we don't have to localize it. 
        ]
        public IntegerUpDown()
        {
            // this class overrides GetPreferredSizeCore, let Control automatically cache the result 
            //SetState2(STATE2_USEPREFERREDSIZECACHE, true);
            Text = "0";
            StopAcceleration();
        }

        ////////////////////////////////////////////////////////////// 
        // Properties
        // 
        ////////////////////////////////////////////////////////////// 


        /// <summary>
        ///     Specifies the acceleration information.
        /// </summary>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public IntegerUpDownAccelerationCollection Accelerations
        {
            get
            {
                if (accelerations == null)
                {
                    accelerations = new IntegerUpDownAccelerationCollection();
                }
                return accelerations;
            }
        }

        /// <summary>
        ///     Gets or sets a value indicating whether the up-down control should
        ///     display the value it contains in hexadecimal format.
        /// </summary>
        public bool Hexadecimal
        {
            get
            {
                return hexadecimal;
            }
            set
            {
                hexadecimal = value;
                UpdateEditText();
            }
        }

        /// <summary>
        ///     Gets or sets the value to increment or decrement
        ///     the up-down control when the up or down buttons are clicked.
        /// </summary>
        public Int32 Increment
        {
            get
            {
                if (accelerationsCurrentIndex != InvalidValue)
                {
                    return Accelerations[accelerationsCurrentIndex].Increment;
                }
                return increment;
            }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("Increment");
                }
                else
                {
                    increment = value;
                }
            }
        }

        /// <summary>
        ///     Gets or sets the maximum value for the up-down control.
        /// </summary>
        public Int32 Maximum
        {
            get
            {
                return maximum;
            }
            set
            {
                maximum = value;
                if (minimum > maximum)
                {
                    minimum = maximum;
                }

                Value = Constrain(currentValue);

                Debug.Assert(maximum == value, "Maximum != what we just set it to!");
            }
        }

        /// <summary>
        ///     Gets or sets the minimum allowed value for the up-down control.
        /// </summary>
        public Int32 Minimum
        {
            get
            {
                return minimum;
            }
            set
            {
                minimum = value;
                if (minimum > maximum)
                {
                    maximum = value;
                }

                Value = Constrain(currentValue);

                Debug.Assert(minimum.Equals(value), "Minimum != what we just set it to!");
            }
        }

        /// <summary>
        ///     [To be supplied.]
        /// </summary>
        [Browsable(false),
         EditorBrowsable(EditorBrowsableState.Never),
         DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public new Padding Padding
        {
            get
            {
                return base.Padding;
            }
            set
            {
                base.Padding = value;
            }
        }

        /// <summary>
        ///     Determines whether the UpDownButtons have been pressed for enough time to activate acceleration.
        /// </summary>
        private bool Spinning
        {
            get
            {
                return accelerations != null && buttonPressedStartTime != InvalidValue;
            }
        }

        /// <summary>
        ///     The text displayed in the control.
        /// </summary>
        [
            Browsable(false), EditorBrowsable(EditorBrowsableState.Never),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        // We're just overriding this to make it non-browsable. 
        public override string Text
        {
            get
            {
                return base.Text;
            }
            set
            {
                base.Text = value;
            }
        }

        /* 
         * The current value of the control 
         */

        /// <summary>
        ///     Gets or sets the value assigned to the up-down control.
        /// </summary>
        public Int32 Value
        {
            get
            {
                if (UserEdit)
                {
                    ValidateEditText();
                }
                return currentValue;
            }

            set
            {
                if (value != currentValue)
                {
                    if (!initializing && ((value < minimum) || (value > maximum)))
                    {
                        throw new ArgumentOutOfRangeException("Value");
                    }
                    else
                    {
                        currentValue = value;

                        OnValueChanged(EventArgs.Empty);
                        currentValueChanged = true;
                        UpdateEditText();
                    }
                }
            }
        }


        //////////////////////////////////////////////////////////////
        // Methods
        //
        ////////////////////////////////////////////////////////////// 

        /// <summary>
        ///     Handles tasks required when the control is being initialized.
        /// </summary>
        public void BeginInit()
        {
            initializing = true;
        }

        /// <summary>
        ///     Called when initialization of the control is complete.
        /// </summary>
        public void EndInit()
        {
            initializing = false;
            Value = Constrain(currentValue);
            UpdateEditText();
        }

        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler PaddingChanged
        {
            add
            {
                base.PaddingChanged += value;
            }
            remove
            {
                base.PaddingChanged -= value;
            }
        }

        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler TextChanged
        {
            add
            {
                base.TextChanged += value;
            }
            remove
            {
                base.TextChanged -= value;
            }
        }

        /// <summary>
        ///     Occurs when the IntegerUpDown.Value property has been changed in some way.
        /// </summary>
        public event EventHandler ValueChanged
        {
            add
            {
                onValueChanged += value;
            }
            remove
            {
                onValueChanged -= value;
            }
        }

        // 
        // Returns the provided value constrained to be within the min and max.
        // 
        private Int32 Constrain(Int32 value)
        {
            Debug.Assert(minimum <= maximum, "minimum > maximum");

            if (value < minimum)
            {
                value = minimum;
            }

            if (value > maximum)
            {
                value = maximum;
            }

            return value;
        }

        /// <summary>
        ///     Decrements the value of the up-down control.
        /// </summary>
        public override void DownButton()
        {
            SetNextAcceleration();

            if (UserEdit)
            {
                ParseEditText();
            }

            var newValue = currentValue;

            // Operations on Decimals can throw OverflowException. 
            //
            try
            {
                newValue -= Increment;

                if (newValue < minimum)
                {
                    newValue = minimum;
                    if (Spinning)
                    {
                        StopAcceleration();
                    }
                }
            }
            catch (OverflowException)
            {
                newValue = minimum;
            }

            Value = newValue;
        }

        /// <summary>
        ///     Overridden to set/reset acceleration variables.
        /// </summary>
        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (base.InterceptArrowKeys && (e.KeyCode == Keys.Up || e.KeyCode == Keys.Down) && !Spinning)
            {
                StartAcceleration();
            }

            base.OnKeyDown(e);
        }

        /// <summary>
        ///     Overridden to set/reset acceleration variables.
        /// </summary>
        protected override void OnKeyUp(KeyEventArgs e)
        {
            if (base.InterceptArrowKeys && (e.KeyCode == Keys.Up || e.KeyCode == Keys.Down))
            {
                StopAcceleration();
            }

            base.OnKeyUp(e);
        }

        /// <summary>
        ///     Restricts the entry of characters to digits (including hex), the negative sign,
        ///     The decimal point, and editing keystrokes (backspace).
        /// </summary>
        protected override void OnTextBoxKeyPress(object source, KeyPressEventArgs e)
        {
            base.OnTextBoxKeyPress(source, e);

            var numberFormatInfo = CultureInfo.CurrentCulture.NumberFormat;
            var decimalSeparator = numberFormatInfo.NumberDecimalSeparator;
            var groupSeparator = numberFormatInfo.NumberGroupSeparator;
            var negativeSign = numberFormatInfo.NegativeSign;

            var keyInput = e.KeyChar.ToString();

            if (Char.IsDigit(e.KeyChar))
            {
                // Digits are OK 
            }
            else if (keyInput.Equals(decimalSeparator) || keyInput.Equals(groupSeparator) ||
                     keyInput.Equals(negativeSign))
            {
                // Int32 separator is OK 
            }
            else if (e.KeyChar == '\b')
            {
                // Backspace key is OK 
            }
            else if (Hexadecimal && ((e.KeyChar >= 'a' && e.KeyChar <= 'f') || e.KeyChar >= 'A' && e.KeyChar <= 'F'))
            {
                // Hexadecimal digits are OK
            }
            else if ((ModifierKeys & (Keys.Control | Keys.Alt)) != 0)
            {
                // Let the edit control handle control and alt key combinations 
            }
            else
            {
                // Eat this invalid key and beep 
                e.Handled = true;
                //SafeNativeMethods.MessageBeep(0);
            }
        }

        /// <summary>
        ///     Raises the IntegerUpDown.OnValueChanged event.
        /// </summary>
        protected virtual void OnValueChanged(EventArgs e)
        {
            // Call the event handler
            onValueChanged?.Invoke(this, e);
        }

        protected override void OnLostFocus(EventArgs e)
        {
            base.OnLostFocus(e);
            if (UserEdit)
            {
                UpdateEditText();
            }
        }

        /// <summary>
        ///     Overridden to start/end acceleration.
        /// </summary>
        internal void OnStartTimer()
        {
            StartAcceleration();
        }

        /// <summary>
        ///     Overridden to start/end acceleration.
        /// </summary>
        internal void OnStopTimer()
        {
            StopAcceleration();
        }

        /// <summary>
        ///     Converts the text displayed in the up-down control to a
        ///     numeric value and evaluates it.
        /// </summary>
        protected void ParseEditText()
        {
            Debug.Assert(UserEdit, "ParseEditText() - UserEdit == false");

            try
            {
                // VSWhidbey 173332: Verify that the user is not starting the string with a "-" 
                // before attempting to set the Value property since a "-" is a valid character with
                // which to start a string representing a negative number. 
                if (!string.IsNullOrEmpty(Text) &&
                    !(Text.Length == 1 && Text == "-"))
                {
                    if (Hexadecimal)
                    {
                        Value = Constrain(Convert.ToInt32(Text, 16));
                    }
                    else
                    {
                        Value = Constrain(Int32.Parse(Text, CultureInfo.CurrentCulture));
                    }
                }
            }
            catch
            {
                // Leave value as it is
            }
            finally
            {
                UserEdit = false;
            }
        }

        /// <summary>
        ///     Updates the index of the UpDownNumericAcceleration entry to use (if needed).
        /// </summary>
        private void SetNextAcceleration()
        {
            // Spinning will check if accelerations is null.
            if (Spinning && accelerationsCurrentIndex < (accelerations.Count - 1))
            { // if index not the last entry ... 
                // Ticks are in 100-nanoseconds (1E-7 seconds). 
                var nowTicks = DateTime.Now.Ticks;
                var buttonPressedElapsedTime = nowTicks - buttonPressedStartTime;
                var accelerationInterval = 10000000L * accelerations[accelerationsCurrentIndex + 1].Seconds; // next entry.

                // If Up/Down button pressed for more than the current acceleration entry interval, get next entry in the accel table.
                if (buttonPressedElapsedTime > accelerationInterval)
                {
                    buttonPressedStartTime = nowTicks;
                    accelerationsCurrentIndex++;
                }
            }
        }

        private void ResetIncrement()
        {
            Increment = DefaultIncrement;
        }

        private void ResetMaximum()
        {
            Maximum = DefaultMaximum;
        }

        private void ResetMinimum()
        {
            Minimum = DefaultMinimum;
        }

        private void ResetValue()
        {
            Value = DefaultValue;
        }

        /// <summary>
        ///     Records when UpDownButtons are pressed to enable acceleration.
        /// </summary>
        private void StartAcceleration()
        {
            buttonPressedStartTime = DateTime.Now.Ticks;
        }

        /// <summary>
        ///     Reset when UpDownButtons are pressed.
        /// </summary>
        private void StopAcceleration()
        {
            accelerationsCurrentIndex = InvalidValue;
            buttonPressedStartTime = InvalidValue;
        }

        /// <summary>
        ///     Provides some interesting info about this control in String form.
        /// </summary>
        public override string ToString()
        {
            var s = base.ToString();
            s += ", Minimum = " + Minimum.ToString(CultureInfo.CurrentCulture) + ", Maximum = " + Maximum.ToString(CultureInfo.CurrentCulture);
            return s;
        }

        /// <summary>
        ///     Increments the value of the up-down control.
        /// </summary>
        public override void UpButton()
        {
            SetNextAcceleration();

            if (UserEdit)
            {
                ParseEditText();
            }

            var newValue = currentValue;

            // Operations on Decimals can throw OverflowException.
            //
            try
            {
                newValue += Increment;

                if (newValue > maximum)
                {
                    newValue = maximum;
                    if (Spinning)
                    {
                        StopAcceleration();
                    }
                }
            }
            catch (OverflowException)
            {
                newValue = maximum;
            }

            Value = newValue;
        }

        private string GetNumberText(Int32 num)
        {
            string text;

            if (Hexadecimal)
            {
                text = ((Int64)num).ToString("X", CultureInfo.InvariantCulture);
                Debug.Assert(text == text.ToUpper(CultureInfo.InvariantCulture), "GetPreferredSize assumes hex digits to be uppercase.");
            }
            else
            {
                text = num.ToString("D", CultureInfo.CurrentCulture);
            }
            return text;
        }

        /// <summary>
        ///     Displays the current value of the up-down control in the appropriate format.
        /// </summary>
        protected override void UpdateEditText()
        {
            // If we're initializing, we don't want to update the edit text yet, 
            // just in case the value is invalid.
            if (initializing)
            {
                return;
            }

            // If the current value is user-edited, then parse this value before reformatting
            if (UserEdit)
            {
                ParseEditText();
            }

            // VSWhidbey 173332: Verify that the user is not starting the string with a "-" 
            // before attempting to set the Value property since a "-" is a valid character with 
            // which to start a string representing a negative number.
            if (currentValueChanged || (!string.IsNullOrEmpty(Text) &&
                                        !(Text.Length == 1 && Text == "-")))
            {
                currentValueChanged = false;
                ChangingText = true;

                // Make sure the current value is within the min/max 
                Debug.Assert(minimum <= currentValue && currentValue <= maximum,
                             "DecimalValue lies outside of [minimum, maximum]");

                Text = GetNumberText(currentValue);
                Debug.Assert(ChangingText == false, "ChangingText should have been set to false");
            }
        }

        /// <summary>
        ///     Validates and updates
        ///     the text displayed in the up-down control.
        /// </summary>
        protected override void ValidateEditText()
        {
            // See if the edit text parses to a valid decimal 
            ParseEditText();
            UpdateEditText();
        }

        private int GetLargestDigit(int start, int end)
        {
            var largestDigit = -1;
            var digitWidth = -1;

            for (var i = start; i < end; ++i)
            {
                char ch;
                if (i < 10)
                {
                    ch = i.ToString(CultureInfo.InvariantCulture)[0];
                }
                else
                {
                    ch = (char)('A' + (i - 10));
                }

                var digitSize = TextRenderer.MeasureText(ch.ToString(), Font);

                if (digitSize.Width >= digitWidth)
                {
                    digitWidth = digitSize.Width;
                    largestDigit = i;
                }
            }
            Debug.Assert(largestDigit != -1 && digitWidth != -1, "Failed to find largest digit.");
            return largestDigit;
        }
    }
}
