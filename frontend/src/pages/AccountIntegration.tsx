import React, { useState, useEffect } from 'react';
import { 
  Mail, 
  MessageCircle, 
  Plus, 
  Trash2, 
  Settings, 
  CheckCircle, 
  AlertCircle,
  Facebook,
  Twitter,
  Instagram,
  MessageSquare,
  Smartphone,
  Loader
} from 'lucide-react';
import { toast } from 'react-hot-toast';
import { 
  accountIntegrationService, 
  ConnectedAccount, 
  ConnectGmailRequest, 
  ConnectWhatsAppRequest 
} from '../services/accountIntegrationService';

const AccountIntegration: React.FC = () => {
  const [connectedAccounts, setConnectedAccounts] = useState<ConnectedAccount[]>([]);
  const [isAddingAccount, setIsAddingAccount] = useState(false);
  const [selectedProvider, setSelectedProvider] = useState<string>('');
  const [isLoading, setIsLoading] = useState(false);
  const [isLoadingAccounts, setIsLoadingAccounts] = useState(true);

  // Form states for different providers
  const [gmailForm, setGmailForm] = useState({ email: '', password: '' });
  const [whatsappForm, setWhatsappForm] = useState({ phoneNumber: '', password: '' });

  const emailProviders = [
    { name: 'Gmail', icon: Mail, color: 'bg-red-500' },
    { name: 'Outlook', icon: Mail, color: 'bg-blue-500' },
    { name: 'Yahoo Mail', icon: Mail, color: 'bg-purple-500' },
    { name: 'ProtonMail', icon: Mail, color: 'bg-orange-500' }
  ];

  const messengerProviders = [
    { name: 'WhatsApp', icon: MessageSquare, color: 'bg-green-500' },
    { name: 'Telegram', icon: Smartphone, color: 'bg-blue-400' },
    { name: 'Facebook Messenger', icon: Facebook, color: 'bg-blue-600' },
    { name: 'Twitter DM', icon: Twitter, color: 'bg-blue-400' },
    { name: 'Instagram DM', icon: Instagram, color: 'bg-pink-500' }
  ];

  // Load connected accounts on component mount
  useEffect(() => {
    loadConnectedAccounts();
  }, []);

  const loadConnectedAccounts = async () => {
    setIsLoadingAccounts(true);
    try {
      const accounts = await accountIntegrationService.getConnectedAccounts();
      setConnectedAccounts(accounts);
    } catch (error) {
      console.error('Failed to load accounts:', error);
    } finally {
      setIsLoadingAccounts(false);
    }
  };

  const handleAddAccount = (provider: string) => {
    setSelectedProvider(provider);
    setIsAddingAccount(true);
  };

  const handleConnectAccount = async () => {
    setIsLoading(true);
    try {
      let success = false;

      if (selectedProvider === 'Gmail') {
        const request: ConnectGmailRequest = {
          email: gmailForm.email,
          password: gmailForm.password
        };
        success = await accountIntegrationService.connectGmail(request);
      } else if (selectedProvider === 'WhatsApp') {
        const request: ConnectWhatsAppRequest = {
          phoneNumber: whatsappForm.phoneNumber,
          password: whatsappForm.password
        };
        success = await accountIntegrationService.connectWhatsApp(request);
      } else {
        // For other providers, show a placeholder message
        toast.success(`${selectedProvider} integration coming soon!`);
        success = true;
      }

      if (success) {
        setIsAddingAccount(false);
        setGmailForm({ email: '', password: '' });
        setWhatsappForm({ phoneNumber: '', password: '' });
        await loadConnectedAccounts(); // Refresh the accounts list
      }
    } catch (error) {
      console.error('Failed to connect account:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const handleRemoveAccount = async (accountId: string) => {
    try {
      const success = await accountIntegrationService.removeAccount(accountId);
      if (success) {
        await loadConnectedAccounts(); // Refresh the accounts list
      }
    } catch (error) {
      console.error('Failed to remove account:', error);
    }
  };

  const handleToggleAccount = async (accountId: string, currentStatus: boolean) => {
    try {
      const success = await accountIntegrationService.toggleAccountStatus(accountId, !currentStatus);
      if (success) {
        await loadConnectedAccounts(); // Refresh the accounts list
      }
    } catch (error) {
      console.error('Failed to toggle account status:', error);
    }
  };

  const formatLastSync = (timestamp: number) => {
    const now = Date.now();
    const diffInSeconds = Math.floor((now - timestamp * 1000) / 1000);
    
    if (diffInSeconds < 60) {
      return 'Just now';
    } else if (diffInSeconds < 3600) {
      return `${Math.floor(diffInSeconds / 60)} minutes ago`;
    } else if (diffInSeconds < 86400) {
      return `${Math.floor(diffInSeconds / 3600)} hours ago`;
    } else {
      return `${Math.floor(diffInSeconds / 86400)} days ago`;
    }
  };

  return (
    <div className="h-screen flex bg-dark-950">
      {/* Sidebar */}
      <div className="w-80 bg-dark-900 border-r border-dark-700 flex flex-col">
        {/* Header */}
        <div className="p-4 border-b border-dark-700">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-lg flex items-center justify-center">
              <Settings className="w-6 h-6 text-white" />
            </div>
            <div>
              <h1 className="text-lg font-semibold text-gradient">Account Integration</h1>
              <p className="text-xs text-dark-400">Manage your connected accounts</p>
            </div>
          </div>
        </div>

        {/* Connected Accounts List */}
        <div className="flex-1 overflow-y-auto p-4">
          <h2 className="text-sm font-medium text-dark-300 mb-4">Connected Accounts</h2>
          
          {isLoadingAccounts ? (
            <div className="text-center py-8">
              <Loader className="w-8 h-8 text-cockpit-400 mx-auto mb-4 animate-spin" />
              <p className="text-dark-400">Loading accounts...</p>
            </div>
          ) : connectedAccounts.length === 0 ? (
            <div className="text-center py-8">
              <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
              <p className="text-dark-400">No accounts connected</p>
              <p className="text-xs text-dark-500 mt-2">Connect your accounts to get started</p>
            </div>
          ) : (
            <div className="space-y-3">
              {connectedAccounts.map((account) => (
                <div key={account.id} className="bg-dark-800 rounded-lg p-3">
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center space-x-3">
                      <div className={`w-8 h-8 rounded-full flex items-center justify-center ${
                        account.provider === 'Gmail' ? 'bg-red-500' :
                        account.provider === 'WhatsApp' ? 'bg-green-500' :
                        'bg-cockpit-500'
                      }`}>
                        {account.provider === 'Gmail' ? <Mail className="w-4 h-4 text-white" /> :
                         account.provider === 'WhatsApp' ? <MessageSquare className="w-4 h-4 text-white" /> :
                         <MessageCircle className="w-4 h-4 text-white" />}
                      </div>
                      <div>
                        <p className="text-sm font-medium text-white">{account.provider}</p>
                        <p className="text-xs text-dark-400">
                          {account.email || account.username}
                        </p>
                      </div>
                    </div>
                    <div className="flex items-center space-x-2">
                      <button
                        onClick={() => handleToggleAccount(account.id, account.isActive)}
                        className={`w-4 h-4 rounded-full border-2 ${
                          account.isActive 
                            ? 'bg-cockpit-500 border-cockpit-500' 
                            : 'bg-transparent border-dark-600'
                        }`}
                      />
                      <button
                        onClick={() => handleRemoveAccount(account.id)}
                        className="p-1 text-dark-400 hover:text-red-400 transition-colors"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    </div>
                  </div>
                  
                  <div className="flex items-center justify-between text-xs text-dark-400">
                    <span>Last sync: {formatLastSync(account.lastSync)}</span>
                    {account.unreadCount && account.unreadCount > 0 && (
                      <span className="bg-cockpit-500 text-white px-2 py-1 rounded-full">
                        {account.unreadCount} new
                      </span>
                    )}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 flex flex-col">
        <div className="p-6">
          <div className="flex items-center justify-between mb-6">
            <h1 className="text-2xl font-bold text-white">Connect Your Accounts</h1>
            <button
              onClick={() => setIsAddingAccount(true)}
              className="btn-primary flex items-center space-x-2"
            >
              <Plus className="w-4 h-4" />
              <span>Add Account</span>
            </button>
          </div>

          {/* Email Providers */}
          <div className="mb-8">
            <h2 className="text-lg font-semibold text-white mb-4 flex items-center">
              <Mail className="w-5 h-5 mr-2 text-cockpit-400" />
              Email Accounts
            </h2>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
              {emailProviders.map((provider) => (
                <div
                  key={provider.name}
                  className="bg-dark-900 border border-dark-700 rounded-lg p-4 hover:border-cockpit-500 transition-colors cursor-pointer"
                  onClick={() => handleAddAccount(provider.name)}
                >
                  <div className="flex items-center space-x-3 mb-3">
                    <div className={`w-10 h-10 rounded-lg flex items-center justify-center ${provider.color}`}>
                      <provider.icon className="w-5 h-5 text-white" />
                    </div>
                    <div>
                      <h3 className="font-medium text-white">{provider.name}</h3>
                      <p className="text-xs text-dark-400">Email service</p>
                    </div>
                  </div>
                  <p className="text-xs text-dark-400">
                    Connect your {provider.name} account to receive emails in Cockpit
                  </p>
                </div>
              ))}
            </div>
          </div>

          {/* Messenger Providers */}
          <div>
            <h2 className="text-lg font-semibold text-white mb-4 flex items-center">
              <MessageCircle className="w-5 h-5 mr-2 text-cockpit-400" />
              Messenger Accounts
            </h2>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
              {messengerProviders.map((provider) => (
                <div
                  key={provider.name}
                  className="bg-dark-900 border border-dark-700 rounded-lg p-4 hover:border-cockpit-500 transition-colors cursor-pointer"
                  onClick={() => handleAddAccount(provider.name)}
                >
                  <div className="flex items-center space-x-3 mb-3">
                    <div className={`w-10 h-10 rounded-lg flex items-center justify-center ${provider.color}`}>
                      <provider.icon className="w-5 h-5 text-white" />
                    </div>
                    <div>
                      <h3 className="font-medium text-white">{provider.name}</h3>
                      <p className="text-xs text-dark-400">Messaging platform</p>
                    </div>
                  </div>
                  <p className="text-xs text-dark-400">
                    Connect your {provider.name} account to receive messages in Cockpit
                  </p>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>

      {/* Add Account Modal */}
      {isAddingAccount && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <div className="bg-dark-900 border border-dark-700 rounded-lg p-6 w-full max-w-md">
            <h2 className="text-xl font-semibold text-white mb-4">
              Connect {selectedProvider}
            </h2>
            <p className="text-dark-400 mb-6">
              Enter your {selectedProvider} credentials to connect your account
            </p>
            
            <div className="space-y-4">
              {selectedProvider === 'Gmail' && (
                <>
                  <div>
                    <label className="block text-sm font-medium text-dark-300 mb-2">
                      Email Address
                    </label>
                    <input
                      type="email"
                      value={gmailForm.email}
                      onChange={(e) => setGmailForm(prev => ({ ...prev, email: e.target.value }))}
                      className="input-field w-full"
                      placeholder="Enter your Gmail address"
                    />
                  </div>
                  
                  <div>
                    <label className="block text-sm font-medium text-dark-300 mb-2">
                      Password
                    </label>
                    <input
                      type="password"
                      value={gmailForm.password}
                      onChange={(e) => setGmailForm(prev => ({ ...prev, password: e.target.value }))}
                      className="input-field w-full"
                      placeholder="Enter your Gmail password"
                    />
                  </div>
                </>
              )}
              
              {selectedProvider === 'WhatsApp' && (
                <>
                  <div>
                    <label className="block text-sm font-medium text-dark-300 mb-2">
                      Phone Number
                    </label>
                    <input
                      type="tel"
                      value={whatsappForm.phoneNumber}
                      onChange={(e) => setWhatsappForm(prev => ({ ...prev, phoneNumber: e.target.value }))}
                      className="input-field w-full"
                      placeholder="Enter your WhatsApp phone number"
                    />
                  </div>
                  
                  <div>
                    <label className="block text-sm font-medium text-dark-300 mb-2">
                      Password
                    </label>
                    <input
                      type="password"
                      value={whatsappForm.password}
                      onChange={(e) => setWhatsappForm(prev => ({ ...prev, password: e.target.value }))}
                      className="input-field w-full"
                      placeholder="Enter your WhatsApp password"
                    />
                  </div>
                </>
              )}
              
              {selectedProvider !== 'Gmail' && selectedProvider !== 'WhatsApp' && (
                <div className="text-center py-8">
                  <AlertCircle className="w-12 h-12 text-yellow-500 mx-auto mb-4" />
                  <p className="text-dark-400">
                    {selectedProvider} integration is coming soon!
                  </p>
                  <p className="text-xs text-dark-500 mt-2">
                    We're working on adding support for {selectedProvider}
                  </p>
                </div>
              )}
            </div>
            
            <div className="flex space-x-3 mt-6">
              <button
                onClick={() => {
                  setIsAddingAccount(false);
                  setGmailForm({ email: '', password: '' });
                  setWhatsappForm({ phoneNumber: '', password: '' });
                }}
                className="btn-secondary flex-1"
                disabled={isLoading}
              >
                Cancel
              </button>
              <button
                onClick={handleConnectAccount}
                disabled={isLoading || (selectedProvider !== 'Gmail' && selectedProvider !== 'WhatsApp')}
                className="btn-primary flex-1 flex items-center justify-center space-x-2"
              >
                {isLoading ? (
                  <>
                    <Loader className="w-4 h-4 animate-spin" />
                    <span>Connecting...</span>
                  </>
                ) : (
                  <span>Connect Account</span>
                )}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default AccountIntegration; 