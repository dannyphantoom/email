import React, { useState, useEffect, useRef } from 'react';
import { Send, Paperclip, Smile, MoreVertical, Download, Upload, Users, MessageCircle } from 'lucide-react';
import { format } from 'date-fns';
import { toast } from 'react-hot-toast';

interface Message {
  id: string;
  content: string;
  sender: string;
  timestamp: Date;
  isOwn: boolean;
  type: 'text' | 'file' | 'image';
}

interface ChatWindowProps {
  chatId: string;
}

const ChatWindow: React.FC<ChatWindowProps> = ({ chatId }) => {
  const [messages, setMessages] = useState<Message[]>([]);
  const [newMessage, setNewMessage] = useState('');
  const [isTyping, setIsTyping] = useState(false);
  const [chatInfo, setChatInfo] = useState<any>(null);
  const [isGroup, setIsGroup] = useState(false);
  const [groupMembers, setGroupMembers] = useState<any[]>([]);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  // Parse chat ID to determine if it's a group or direct chat
  useEffect(() => {
    if (chatId.startsWith('group-')) {
      setIsGroup(true);
      const groupId = chatId.replace('group-', '');
      fetchGroupInfo(groupId);
      fetchGroupMessages(groupId);
    } else if (chatId.startsWith('chat-')) {
      setIsGroup(false);
      const sessionId = chatId.replace('chat-', '');
      fetchDirectMessages(sessionId);
    }
  }, [chatId]);

  const fetchGroupInfo = async (groupId: string) => {
    try {
      const response = await fetch(`/api/groups/${groupId}`, {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        setChatInfo(data.data);
      }
    } catch (error) {
      console.error('Failed to fetch group info:', error);
    }
  };

  const fetchGroupMessages = async (groupId: string) => {
    try {
      const response = await fetch(`/api/groups/${groupId}/messages`, {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const formattedMessages = data.data.map((msg: any) => ({
          id: msg.id.toString(),
          content: msg.content,
          sender: msg.sender_name || 'Unknown',
          timestamp: new Date(msg.timestamp),
          isOwn: msg.sender_id === getCurrentUserId(),
          type: msg.message_type || 'text'
        }));
        setMessages(formattedMessages);
      }
    } catch (error) {
      console.error('Failed to fetch group messages:', error);
      toast.error('Failed to load messages');
    }
  };

  const fetchDirectMessages = async (sessionId: string) => {
    try {
      const response = await fetch(`/api/messages/${sessionId}`, {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const formattedMessages = data.data.map((msg: any) => ({
          id: msg.id.toString(),
          content: msg.content,
          sender: msg.sender_name || 'Unknown',
          timestamp: new Date(msg.timestamp),
          isOwn: msg.sender_id === getCurrentUserId(),
          type: msg.message_type || 'text'
        }));
        setMessages(formattedMessages);
      }
    } catch (error) {
      console.error('Failed to fetch direct messages:', error);
      toast.error('Failed to load messages');
    }
  };

  const getCurrentUserId = () => {
    // This should come from your auth store
    return localStorage.getItem('userId') || '1';
  };

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSendMessage = async () => {
    if (!newMessage.trim()) return;

    const messageData = {
      content: newMessage,
      type: 'text'
    };

    try {
      let response;
      if (isGroup) {
        const groupId = chatId.replace('group-', '');
        response = await fetch(`/api/groups/${groupId}/messages`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${localStorage.getItem('token')}`
          },
          body: JSON.stringify(messageData)
        });
      } else {
        const sessionId = chatId.replace('chat-', '');
        response = await fetch(`/api/messages/${sessionId}`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${localStorage.getItem('token')}`
          },
          body: JSON.stringify(messageData)
        });
      }

      if (response.ok) {
        const message: Message = {
          id: Date.now().toString(),
          content: newMessage,
          sender: 'You',
          timestamp: new Date(),
          isOwn: true,
          type: 'text',
        };
        setMessages(prev => [...prev, message]);
        setNewMessage('');
      } else {
        toast.error('Failed to send message');
      }
    } catch (error) {
      console.error('Failed to send message:', error);
      toast.error('Failed to send message');
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  const handleBackupChat = async () => {
    const backupName = prompt('Enter backup name:');
    if (!backupName) return;

    const backupData = {
      chatId,
      messages,
      chatInfo,
      timestamp: new Date().toISOString()
    };

    try {
      const response = await fetch('/api/backup', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        },
        body: JSON.stringify({
          name: backupName,
          data: JSON.stringify(backupData),
          description: `Backup of ${isGroup ? 'group' : 'chat'} conversation`
        })
      });

      if (response.ok) {
        toast.success('Chat backup created successfully');
      } else {
        toast.error('Failed to create backup');
      }
    } catch (error) {
      console.error('Failed to create backup:', error);
      toast.error('Failed to create backup');
    }
  };

  const handleRestoreChat = async () => {
    try {
      const response = await fetch('/api/backup', {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });

      if (response.ok) {
        const data = await response.json();
        const backups = data.data;
        
        if (backups.length === 0) {
          toast.error('No backups available');
          return;
        }

        // Show backup selection dialog
        const backupNames = backups.map((b: any) => b.name);
        const selectedBackup = prompt('Select backup to restore:\n' + backupNames.join('\n'));
        
        if (!selectedBackup) return;

        const backup = backups.find((b: any) => b.name === selectedBackup);
        if (!backup) {
          toast.error('Backup not found');
          return;
        }

        const restoreResponse = await fetch(`/api/backup/${backup.id}/restore`, {
          method: 'POST',
          headers: {
            'Authorization': `Bearer ${localStorage.getItem('token')}`
          }
        });

        if (restoreResponse.ok) {
          toast.success('Chat restored successfully');
          // Reload messages
          if (isGroup) {
            const groupId = chatId.replace('group-', '');
            fetchGroupMessages(groupId);
          } else {
            const sessionId = chatId.replace('chat-', '');
            fetchDirectMessages(sessionId);
          }
        } else {
          toast.error('Failed to restore chat');
        }
      }
    } catch (error) {
      console.error('Failed to restore chat:', error);
      toast.error('Failed to restore chat');
    }
  };

  return (
    <div className="flex-1 flex flex-col">
      {/* Chat Header */}
      <div className="bg-dark-900 border-b border-dark-700 p-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
              {isGroup ? (
                <Users className="w-5 h-5 text-white" />
              ) : (
                <span className="text-white font-medium text-sm">
                  {chatInfo?.name?.charAt(0).toUpperCase() || 'U'}
                </span>
              )}
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">
                {chatInfo?.name || 'Loading...'}
              </h2>
              <p className="text-sm text-dark-400">
                {isTyping ? 'Typing...' : (isGroup ? `${groupMembers.length} members` : 'Online')}
              </p>
            </div>
          </div>
          <div className="flex items-center space-x-2">
            <button
              onClick={handleBackupChat}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
              title="Backup Chat"
            >
              <Download className="w-4 h-4" />
            </button>
            <button
              onClick={handleRestoreChat}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
              title="Restore Chat"
            >
              <Upload className="w-4 h-4" />
            </button>
            <button className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors">
              <MoreVertical className="w-5 h-5" />
            </button>
          </div>
        </div>
      </div>

      {/* Messages Area */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 chat-mesh-bg">
        {messages.length === 0 ? (
          <div className="text-center py-8">
            <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
            <p className="text-dark-400">No messages yet</p>
            <p className="text-sm text-dark-500">Start the conversation!</p>
          </div>
        ) : (
          messages.map((message) => (
            <div
              key={message.id}
              className={`flex ${message.isOwn ? 'justify-end' : 'justify-start'}`}
            >
              <div className={`message-bubble ${
                message.isOwn ? 'message-bubble-sent' : 'message-bubble-received'
              }`}>
                <p className="text-sm">{message.content}</p>
                <p className={`text-xs mt-1 ${
                  message.isOwn ? 'text-white/70' : 'text-dark-300'
                }`}>
                  {format(message.timestamp, 'HH:mm')}
                </p>
              </div>
            </div>
          ))
        )}
        <div ref={messagesEndRef} />
      </div>

      {/* Message Input */}
      <div className="bg-dark-900 border-t border-dark-700 p-4">
        <div className="flex items-end space-x-3">
          <div className="flex-1 relative">
            <textarea
              value={newMessage}
              onChange={(e) => setNewMessage(e.target.value)}
              onKeyPress={handleKeyPress}
              placeholder="Type a message..."
              className="input-field w-full resize-none min-h-[44px] max-h-32 py-3 pr-20"
              rows={1}
            />
            <div className="absolute right-2 bottom-2 flex items-center space-x-1">
              <button className="p-1 text-dark-400 hover:text-white transition-colors">
                <Paperclip className="w-4 h-4" />
              </button>
              <button className="p-1 text-dark-400 hover:text-white transition-colors">
                <Smile className="w-4 h-4" />
              </button>
            </div>
          </div>
          <button
            onClick={handleSendMessage}
            disabled={!newMessage.trim()}
            className="btn-primary p-3 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <Send className="w-5 h-5" />
          </button>
        </div>
      </div>
    </div>
  );
};

export default ChatWindow; 