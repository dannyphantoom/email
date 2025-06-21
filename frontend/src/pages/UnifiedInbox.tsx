import React, { useState, useEffect } from 'react';
import { 
  Mail, 
  MessageCircle, 
  Settings, 
  RefreshCw, 
  Plus,
  Inbox,
  Star,
  Send,
  Archive,
  Trash2
} from 'lucide-react';
import UnifiedMessageList from '../components/UnifiedMessageList';
import { toast } from 'react-hot-toast';

interface UnifiedMessage {
  id: string;
  accountId: string;
  sender: string;
  recipient: string;
  subject: string;
  content: string;
  messageType: 'email' | 'message' | 'notification';
  timestamp: string;
  isRead: boolean;
  isImportant: boolean;
  attachments: string[];
  metadata: Record<string, string>;
  accountProvider: string;
}

interface ConnectedAccount {
  id: string;
  type: 'email' | 'messenger';
  provider: string;
  email?: string;
  username?: string;
  isActive: boolean;
  lastSync: string;
  unreadCount: number;
}

const UnifiedInbox: React.FC = () => {
  const [messages, setMessages] = useState<UnifiedMessage[]>([]);
  const [connectedAccounts, setConnectedAccounts] = useState<ConnectedAccount[]>([]);
  const [selectedMessage, setSelectedMessage] = useState<UnifiedMessage | null>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [activeTab, setActiveTab] = useState<'inbox' | 'starred' | 'sent' | 'archive'>('inbox');

  useEffect(() => {
    fetchData();
  }, []);

  const fetchData = async () => {
    setIsLoading(true);
    try {
      // Fetch unified messages
      const messagesRes = await fetch('/api/integration/messages', {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      let messagesData: UnifiedMessage[] = [];
      if (messagesRes.ok) {
        const data = await messagesRes.json();
        messagesData = data.data || [];
      } else {
        toast.error('Failed to load messages');
      }
      // Fetch connected accounts
      const accountsRes = await fetch('/api/integration/accounts', {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      let accountsData: ConnectedAccount[] = [];
      if (accountsRes.ok) {
        const data = await accountsRes.json();
        accountsData = data.data || [];
      } else {
        toast.error('Failed to load accounts');
      }
      setMessages(messagesData);
      setConnectedAccounts(accountsData);
    } catch (error) {
      toast.error('Failed to load unified inbox');
    } finally {
      setIsLoading(false);
    }
  };

  const handleRefresh = async () => {
    fetchData();
    toast.success('Messages refreshed successfully');
  };

  const handleMessageSelect = (message: UnifiedMessage) => {
    setSelectedMessage(message);
  };

  const handleMarkAsRead = (messageId: string) => {
    setMessages(prev => 
      prev.map(msg => 
        msg.id === messageId ? { ...msg, isRead: true } : msg
      )
    );
    toast.success('Message marked as read');
  };

  const handleMarkAsImportant = (messageId: string) => {
    setMessages(prev => 
      prev.map(msg => 
        msg.id === messageId ? { ...msg, isImportant: !msg.isImportant } : msg
      )
    );
    toast.success('Message importance toggled');
  };

  const handleDeleteMessage = (messageId: string) => {
    setMessages(prev => prev.filter(msg => msg.id !== messageId));
    if (selectedMessage?.id === messageId) {
      setSelectedMessage(null);
    }
    toast.success('Message deleted');
  };

  const handleReplyToMessage = (messageId: string) => {
    const message = messages.find(msg => msg.id === messageId);
    if (message) {
      toast.success(`Opening reply to: ${message.subject}`);
      // TODO: Implement reply functionality
    }
  };

  const getFilteredMessages = () => {
    switch (activeTab) {
      case 'starred':
        return messages.filter(msg => msg.isImportant);
      case 'sent':
        return messages.filter(msg => msg.metadata.sent === 'true');
      case 'archive':
        return messages.filter(msg => msg.metadata.archived === 'true');
      default:
        return messages;
    }
  };

  const getUnreadCount = () => {
    return messages.filter(msg => !msg.isRead).length;
  };

  const getImportantCount = () => {
    return messages.filter(msg => msg.isImportant).length;
  };

  return (
    <div className="h-screen flex bg-dark-950">
      {/* Sidebar */}
      <div className="w-80 bg-dark-900 border-r border-dark-700 flex flex-col">
        {/* Header */}
        <div className="p-4 border-b border-dark-700">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-lg flex items-center justify-center">
                <Inbox className="w-6 h-6 text-white" />
              </div>
              <div>
                <h1 className="text-lg font-semibold text-gradient">Unified Inbox</h1>
                <p className="text-xs text-dark-400">All your messages in one place</p>
              </div>
            </div>
            <button
              onClick={handleRefresh}
              disabled={isLoading}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors disabled:opacity-50"
            >
              <RefreshCw className={`w-5 h-5 ${isLoading ? 'animate-spin' : ''}`} />
            </button>
          </div>
        </div>

        {/* Navigation Tabs */}
        <div className="p-4">
          <nav className="space-y-1">
            <button
              onClick={() => setActiveTab('inbox')}
              className={`w-full flex items-center space-x-3 px-3 py-2 rounded-lg transition-colors ${
                activeTab === 'inbox'
                  ? 'bg-cockpit-900 text-cockpit-400 border border-cockpit-500'
                  : 'text-dark-400 hover:text-white hover:bg-dark-800'
              }`}
            >
              <Inbox className="w-5 h-5" />
              <span className="flex-1 text-left">Inbox</span>
              {getUnreadCount() > 0 && (
                <span className="bg-cockpit-500 text-white text-xs px-2 py-1 rounded-full">
                  {getUnreadCount()}
                </span>
              )}
            </button>

            <button
              onClick={() => setActiveTab('starred')}
              className={`w-full flex items-center space-x-3 px-3 py-2 rounded-lg transition-colors ${
                activeTab === 'starred'
                  ? 'bg-cockpit-900 text-cockpit-400 border border-cockpit-500'
                  : 'text-dark-400 hover:text-white hover:bg-dark-800'
              }`}
            >
              <Star className="w-5 h-5" />
              <span className="flex-1 text-left">Starred</span>
              {getImportantCount() > 0 && (
                <span className="bg-yellow-500 text-white text-xs px-2 py-1 rounded-full">
                  {getImportantCount()}
                </span>
              )}
            </button>

            <button
              onClick={() => setActiveTab('sent')}
              className={`w-full flex items-center space-x-3 px-3 py-2 rounded-lg transition-colors ${
                activeTab === 'sent'
                  ? 'bg-cockpit-900 text-cockpit-400 border border-cockpit-500'
                  : 'text-dark-400 hover:text-white hover:bg-dark-800'
              }`}
            >
              <Send className="w-5 h-5" />
              <span className="flex-1 text-left">Sent</span>
            </button>

            <button
              onClick={() => setActiveTab('archive')}
              className={`w-full flex items-center space-x-3 px-3 py-2 rounded-lg transition-colors ${
                activeTab === 'archive'
                  ? 'bg-cockpit-900 text-cockpit-400 border border-cockpit-500'
                  : 'text-dark-400 hover:text-white hover:bg-dark-800'
              }`}
            >
              <Archive className="w-5 h-5" />
              <span className="flex-1 text-left">Archive</span>
            </button>
          </nav>
        </div>

        {/* Connected Accounts */}
        <div className="flex-1 overflow-y-auto p-4">
          <h2 className="text-sm font-medium text-dark-300 mb-4">Connected Accounts</h2>
          <div className="space-y-2">
            {connectedAccounts.map((account) => (
              <div key={account.id} className="flex items-center space-x-3 p-2 rounded-lg bg-dark-800">
                <div className={`w-3 h-3 rounded-full ${
                  account.isActive ? 'bg-green-500' : 'bg-red-500'
                }`} />
                <div className="flex-1 min-w-0">
                  <p className="text-sm font-medium text-white truncate">
                    {account.provider}
                  </p>
                  <p className="text-xs text-dark-400 truncate">
                    {account.email || account.username}
                  </p>
                </div>
                {account.unreadCount > 0 && (
                  <span className="bg-cockpit-500 text-white text-xs px-2 py-1 rounded-full">
                    {account.unreadCount}
                  </span>
                )}
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 flex flex-col">
        <UnifiedMessageList
          messages={getFilteredMessages()}
          onMessageSelect={handleMessageSelect}
          onMarkAsRead={handleMarkAsRead}
          onMarkAsImportant={handleMarkAsImportant}
          onDeleteMessage={handleDeleteMessage}
          onReplyToMessage={handleReplyToMessage}
        />
      </div>

      {/* Message Detail Panel */}
      {selectedMessage && (
        <div className="w-96 bg-dark-900 border-l border-dark-700 flex flex-col">
          <div className="p-4 border-b border-dark-700">
            <div className="flex items-center justify-between">
              <h3 className="text-lg font-semibold text-white">Message Details</h3>
              <button
                onClick={() => setSelectedMessage(null)}
                className="text-dark-400 hover:text-white"
              >
                ×
              </button>
            </div>
          </div>
          
          <div className="flex-1 overflow-y-auto p-4">
            <div className="space-y-4">
              <div>
                <h4 className="text-sm font-medium text-dark-300 mb-1">Subject</h4>
                <p className="text-white">{selectedMessage.subject}</p>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-dark-300 mb-1">From</h4>
                <p className="text-white">{selectedMessage.sender}</p>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-dark-300 mb-1">To</h4>
                <p className="text-white">{selectedMessage.recipient}</p>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-dark-300 mb-1">Account</h4>
                <p className="text-white">{selectedMessage.accountProvider}</p>
              </div>
              
              <div>
                <h4 className="text-sm font-medium text-dark-300 mb-1">Content</h4>
                <div className="bg-dark-800 rounded-lg p-3">
                  <p className="text-white whitespace-pre-wrap">{selectedMessage.content}</p>
                </div>
              </div>
              
              {selectedMessage.attachments.length > 0 && (
                <div>
                  <h4 className="text-sm font-medium text-dark-300 mb-1">Attachments</h4>
                  <div className="space-y-2">
                    {selectedMessage.attachments.map((attachment, index) => (
                      <div key={index} className="flex items-center space-x-2 p-2 bg-dark-800 rounded">
                        <span className="text-cockpit-400">📎</span>
                        <span className="text-white text-sm">{attachment}</span>
                      </div>
                    ))}
                  </div>
                </div>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default UnifiedInbox; 